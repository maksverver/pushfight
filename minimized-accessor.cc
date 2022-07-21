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

// Accessor used to look up the result of positions in minized.bin.
std::optional<MinimizedAccessor::successors_t>
MinimizedAccessor::LookupSuccessors(std::string_view perm_string, std::string *error) {
  auto perm = ParsePerm(perm_string, error);
  if (!perm) return {};
  return LookupSuccessors(*perm, error);
}

std::optional<MinimizedAccessor::successors_t>
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

  const int64_t min_index =
      type == PermType::IN_PROGRESS && IsReachable(perm) ? MinIndexOf(perm) : -1;
  const Value stored_value =
      min_index >= 0 ? Value(acc[min_index]) : Value::Tie();

  std::vector<std::pair<Moves, State>> successors = GenerateAllSuccessors(perm);
  Deduplicate(successors);

  std::vector<std::pair<Value, std::pair<Moves, State>>> evaluated_successors;
  for (const std::pair<Moves, State> &elem : successors) {
    const Outcome o = elem.second.outcome;
    const Perm &p = elem.second.perm;
    Value value;
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
      int64_t min_index = MinIndexOf(elem.second.perm);
      value = -Value(acc[min_index]);
    }
    evaluated_successors.push_back({value, elem});
  };

  // Sort by descending value, so that the best moves come first.
  std::sort(evaluated_successors.begin(), evaluated_successors.end(),
    [](
        const std::pair<Value, std::pair<Moves, State>> &a,
        const std::pair<Value, std::pair<Moves, State>> &b) {
      return a.first < b.first;
    });

  if (evaluated_successors.empty()) {
    assert(stored_value == Value::LossIn(0));
  } else {
    assert(min_index == -1 || stored_value == evaluated_successors.front().first);
  }
  return evaluated_successors;
}
