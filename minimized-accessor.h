#ifndef MINIMIZED_ACCESSOR_H_INCLUDED
#define MINIMIZED_ACCESSOR_H_INCLUDED

#include <optional>
#include <string>
#include <string_view>

#include "accessors.h"
#include "perms.h"
#include "position-value.h"

// Accessor used to look up the result of positions in minized.bin.
class MinimizedAccessor {
public:
  explicit MinimizedAccessor(const char *filename)
      : acc(filename) {}

  // List of distinct successors; sorted by decreasing value, with one set of moves per result.
  using successors_t = std::vector<std::pair<Value, std::pair<Moves, State>>>;

  // Looks up the status of the successors of the given permutation string.
  //
  // Returns a list of successors, sorted by value (best first); this list may
  // be empty if there are no valid moves in the given position. If the
  // permutation string was invalid or represented a finished position, an empty
  // optional is returned instead. In that case, if `error` is not null, an error
  // message will be written to *error.
  std::optional<successors_t> LookupSuccessors(std::string_view perm_string, std::string *error);

  // Similar to LookupSuccessor above, but starts from a parsed permutation.
  std::optional<successors_t> LookupSuccessors(const Perm &perm, std::string *error);

private:
  MappedFile<uint8_t, min_index_size> acc;
};

#endif  // ndef MINIMIZED_ACCESSOR_H_INCLUDED
