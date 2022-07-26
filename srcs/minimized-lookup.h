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
std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(const MinimizedAccessor &acc, std::string_view perm_string, std::string *error);

// Similar to LookupSuccessor above, but starts from a parsed permutation.
std::optional<std::vector<EvaluatedSuccessor>>
LookupSuccessors(const MinimizedAccessor &acc, const Perm &perm, std::string *error);

#endif  // ndef MINIMIZED_LOOKUP_H_INCLUDED
