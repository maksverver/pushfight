#ifndef MINIMIZED_LOOKUP_H_INCLUDED
#define MINIMIZED_LOOKUP_H_INCLUDED

#include <optional>
#include <string>
#include <string_view>
#include <utility>
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
LookupSuccessors(const MinimizedAccessor &acc, const Perm &perm, std::string *error);

// Like LookupSuccessors() above, but if `include_successor_values` is true,
// also looks up the values of the successors of each successor.
//
// The successor values are relative to the opponent. For example, if a
// successor has value W3, then the values of its successors may be L2 or L1.
// Successor values are ordered from best to worst, and they are not
// deduplicated.
std::optional<std::vector<std::pair<EvaluatedSuccessor, std::vector<Value>>>>
LookupDetailedSuccessors(
    const MinimizedAccessor &acc,
    const Perm &perm,
    bool include_successor_values,
    std::string *error);

// Converts a sorted sequence of values (as returned by
// LookupDetailedSuccessors(), for example), to a comma-separated string with
// duplicates compressed.
//
// For example, {T, T, L9, L5, L5, L4} is converted to: "T*2,L9*1,L5*2,L4*1".
std::string SuccessorValuesToString(const std::vector<Value> &values);

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

// Looks up the values of successors of multiple permutations.
//
// The result is a vector with size equal to `perms`, where each element is
// itself a vector of values that's equal to the values that would have been
// returned by LookupSuccessors() for the corresponding permutation.
//
// The advantage of this function over calling LookupSuccessors() repeatedly
// is that looking up the successor values of multiple permutations can be much
// more efficient, especially when the MinimizedAccessor is using a compressed
// data file.
std::vector<std::vector<Value>> LookupSuccessorValues(
  const MinimizedAccessor &acc, const std::vector<Perm> &perms);

#endif  // ndef MINIMIZED_LOOKUP_H_INCLUDED
