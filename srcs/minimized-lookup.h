#ifndef MINIMIZED_LOOKUP_H_INCLUDED
#define MINIMIZED_LOOKUP_H_INCLUDED

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "board.h"
#include "minimized-accessor.h"
#include "position-value.h"

struct EvaluatedSuccessor {
  Moves moves;
  State state;
  int64_t min_index;
  bool rotated;
  Value value;
};

inline bool operator<(const EvaluatedSuccessor &a, const EvaluatedSuccessor &b) {
  return a.value < b.value;
}

// Looks up the status of the successors of the given permutation string.
//
// Returns a list of successors, sorted by value (best first); this list may
// be empty if there are no valid moves in the given position. If the
// permutation string was invalid or represented a finished position, an empty
// optional is returned instead. In that case, if `error` is not null, an error
// message will be written to *error.
//
// The returned values are for the previous player (e.g., if a successor state
// is lost-in-X, the value returned will be win-in-(X + 1), and so on.)
std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(const MinimizedAccessor &acc, std::string_view perm_string, std::string *error);

// Similar to LookupSuccessor above, but starts from a parsed permutation.
std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(const MinimizedAccessor &acc, const Perm &perm, std::string *error);

// Calculates the value of the given permutation, without successor information.
//
// The given permutation does not need to be reachable, but it must be valid and
// not finished. If it is reachable, the lookup is very quick since it can be
// found in the minimized accessor directly. Otherwise, this function calls
// RecalculateValue() to calculate the value from the successors.
std::optional<Value> LookupValue(const MinimizedAccessor &acc, const Perm &perm, std::string *error);

// Recalculates the value of the given permutation by examining its successors.
//
// This is similar to LookupSuccessors(), defined above, except that this
// function is often more efficient, since it doesn't have to look up any
// positions in the accessor if a win-in-1 is found.
//
// The given permutation must be a valid starting or in-progress permutation.
//
// The `offsets` and `bytes` objects are used as scratch space; this is useful
// to minimize allocations when RecalculateValue() is called many times. If you
// don't care about this, call the overload below instead.
Value RecalculateValue(
  const MinimizedAccessor &acc,
  const Perm &perm,
  std::vector<int64_t> &offsets,
  std::vector<uint8_t> &bytes);


inline Value RecalculateValue(const MinimizedAccessor &acc, const Perm &perm) {
  std::vector<int64_t> offsets;
  std::vector<uint8_t> bytes;
  return RecalculateValue(acc, perm, offsets, bytes);
}

#endif  // ndef MINIMIZED_LOOKUP_H_INCLUDED
