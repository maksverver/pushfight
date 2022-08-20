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
// found in the minimized accessor directly. If it's not reachable, then the
// successors are evaluated to calculate the value, similar to
// LookupSuccessors() defined above, except that this function is often more
// efficient, since it doesn't have to do lookups in the accessor if a win-in-1
// is found.
std::optional<Value> LookupValue(const MinimizedAccessor &acc, const Perm &perm, std::string *error);

#endif  // ndef MINIMIZED_LOOKUP_H_INCLUDED
