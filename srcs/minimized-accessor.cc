#include "minimized-accessor.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>

#include "accessors.h"
#include "board.h"
#include "perms.h"
#include "position-value.h"
#include "parse-perm.h"
#include "search.h"

#include <iostream>

// Accessor used to look up the result of positions in minized.bin.
std::optional<std::vector<EvaluatedSuccessor>>
MinimizedAccessor::LookupSuccessors(std::string_view perm_string, std::string *error) {
  auto perm = ParsePerm(perm_string, error);
  if (!perm) return {};
  return LookupSuccessors(*perm, error);
}

std::optional<std::vector<EvaluatedSuccessor>>
MinimizedAccessor::LookupSuccessors(const Perm &perm, std::string *error) {
  // Parse permutation argument.
  PermType type = ValidatePerm(perm);
  if (type == PermType::INVALID) {
    if (error) *error = "Permutation is invalid";
    return {};
  }
  if (type == PermType::FINISHED) {
    if (error) *error = "Permutation represents a finished position";
  }
  assert(type == PermType::STARTED || type == PermType::IN_PROGRESS);

  const int64_t init_min_index =
      type == PermType::IN_PROGRESS && IsReachable(perm) ? MinIndexOf(perm) : -1;

  std::vector<std::pair<Moves, State>> successors = GenerateAllSuccessors(perm);
  Deduplicate(successors);

  std::vector<EvaluatedSuccessor> evaluated_successors;
  std::vector<int> incomplete;  // indices of evaluated_successors
  for (const std::pair<Moves, State> &elem : successors) {
    bool rotated = false;
    int64_t min_index = -1;
    const Outcome o = elem.second.outcome;
    const Perm &p = elem.second.perm;
    Value value = Value::Tie();
    if (o == LOSS) {
      assert(IsFinished(p));
      // If the successor is losing for the next player, the moves were
      // winning for the last player.
      value = Value::WinIn(1);
    } else if (o == WIN) {
      assert(IsFinished(p));
      // Symmetric to above. Currently GenerateAllSuccessors() does not return
      // losing moves, so this code never executes.
      value = Value::LossIn(1);
    } else {
      assert(o == TIE);
      assert(IsInProgress(p));
      assert(IsReachable(p));
      min_index = MinIndexOf(elem.second.perm, &rotated);
      incomplete.push_back(evaluated_successors.size());
    }

    evaluated_successors.push_back({
      .moves = elem.first,
      .state = elem.second,
      .min_index = min_index,
      .rotated = rotated,
      .value = value,
    });
  };

  if (init_min_index >= 0) {
    // Add a dummy element corresponding to the initial position,
    // so we can lookup its value together with the missing successors.
    incomplete.push_back(evaluated_successors.size());
    evaluated_successors.push_back({
      .moves = {},
      .state = {},
      .min_index = init_min_index,
      .rotated = false,
      .value = Value::Tie(),
    });
  }

  // Look up successor min-indices for outcomes that are not yet determined.
  const size_t n = incomplete.size();
  // Sort by min_index to improve locality of reference during lookup.
  std::sort(incomplete.begin(), incomplete.end(), [&](int i, int j) {
    return evaluated_successors[i].min_index < evaluated_successors[j].min_index;
  });
  std::vector<int64_t> offsets;
  offsets.reserve(n);
  for (int i : incomplete) offsets.push_back(evaluated_successors[i].min_index);
  std::vector<uint8_t> bytes = ReadBytes(offsets);
  for (size_t i = 0; i < n; ++i) {
    auto &elem = evaluated_successors[incomplete[i]];
    assert(elem.state.outcome == TIE);
    assert(elem.value == Value::Tie());
    elem.value = Value(bytes[i]).ToPredecessor();
  }

  Value stored_value = Value::Tie();
  if (init_min_index >= 0) {
    // Pop dummy element.
    bool lossy = false;
    stored_value = evaluated_successors.back().value.ToSuccessor(&lossy);
    assert(!lossy);
    evaluated_successors.pop_back();
  }

  // Sort by descending value, so that the best moves come first.
  std::sort(evaluated_successors.begin(), evaluated_successors.end());
  if (evaluated_successors.empty()) {
    assert(stored_value == Value::LossIn(0));
  } else {
    assert(init_min_index == -1 || stored_value == evaluated_successors.front().value);
  }
  return evaluated_successors;
}

std::vector<uint8_t> MinimizedAccessor::ReadBytes(const std::vector<int64_t> &offsets) {
  size_t n = offsets.size();
  std::vector<uint8_t> bytes(n);
  for (size_t i = 0; i < n; ++i) {
    bytes[i] = acc[offsets[i]];
  }
  return bytes;
}
