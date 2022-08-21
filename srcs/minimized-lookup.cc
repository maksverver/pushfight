#include "minimized-lookup.h"

#include <cassert>
#include <optional>
#include <string>
#include <string_view>

#include "accessors.h"
#include "board.h"
#include "dedupe.h"
#include "perms.h"
#include "position-value.h"
#include "parse-perm.h"
#include "search.h"

namespace {

// If the perm is invalid or finished, returns an empty optional and if error
// is not null, also assigns an error message to *error.
//
// Otherwise, this function returns the minimized index (if the permutation is
// both in-progress and reachable), or -1 (if the permutation is started or
// or unreachable).
std::optional<int64_t> CheckPermType(const Perm &perm, std::string *error) {
  PermType type = ValidatePerm(perm);
  if (type == PermType::INVALID) {
    if (error) *error = "Permutation is invalid";
    return {};
  }
  if (type == PermType::FINISHED) {
    if (error) *error = "Permutation represents a finished position";
    return {};
  }
  assert(type == PermType::STARTED || type == PermType::IN_PROGRESS);
  return type == PermType::IN_PROGRESS && IsReachable(perm) ? MinIndexOf(perm) : -1;
}

}  // namespace

std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(
    const MinimizedAccessor &acc,
    std::string_view perm_string,
    std::string *error) {
  auto perm = ParsePerm(perm_string, error);
  if (!perm) return {};
  return LookupSuccessors(acc, *perm, error);
}

std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(
    const MinimizedAccessor &acc, const Perm &perm, std::string *error) {
  std::optional<int64_t> opt_init_min_index = CheckPermType(perm, error);
  if (!opt_init_min_index) return {};
  const int64_t init_min_index = *opt_init_min_index;

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
      min_index = MinIndexOf(p, &rotated);
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
  std::vector<uint8_t> bytes = acc.ReadBytes(offsets);
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

std::optional<Value>
LookupValue(const MinimizedAccessor &acc, const Perm &perm, std::string *error) {
  if (std::optional<int64_t> min_index = CheckPermType(perm, error); !min_index) {
    // Invalid argument.
    return {};
  } else if (*min_index >= 0) {
    // Look up value by min-index. This should be pretty fast.
    return Value(acc.ReadByte(*min_index));
  } else {
    // Value is not stored in the minimized file. Recalculate it from successors.
    return RecalculateValue(acc, perm);
  }
}

Value RecalculateValue(
    const MinimizedAccessor &acc,
    const Perm &perm,
    std::vector<int64_t> &offsets,
    std::vector<uint8_t> &bytes) {
  offsets.resize(0);
  Value best_value = Value::LossIn(0);
  if (!GenerateSuccessors(perm, [&](const Moves &, const State &state) {
    if (state.outcome == LOSS) {
      // Win in 1 is the best value possible, so abort the search.
      return false;
    } else if (state.outcome == WIN) {
      // Currently, GenerateAllSuccessors() does not return losing moves,
      // so this code never executes.
      best_value = Value::LossIn(1);
    } else {
      assert(state.outcome == TIE);
      offsets.push_back(MinIndexOf(state.perm, nullptr));
    }
    return true;  // continue
  })) {
    // Win-in-1 found.
    return Value::WinIn(1);
  }

  if (!offsets.empty()) {
    SortAndDedupe(offsets);
    bytes.resize(offsets.size());
    acc.ReadBytes(offsets.data(), bytes.data(), offsets.size());
    for (uint8_t byte : bytes) {
      best_value = std::min(best_value, Value(byte).ToPredecessor());
    }
  }

  return best_value;
}
