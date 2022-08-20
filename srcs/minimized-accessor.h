#ifndef MINIMIZED_ACCESSOR_H_INCLUDED
#define MINIMIZED_ACCESSOR_H_INCLUDED

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "accessors.h"
#include "perms.h"
#include "position-value.h"
#include "xz-accessor.h"

using MappedMinIndex = MappedFile<uint8_t, min_index_size>;

// Accessor used to look up the result of positions in either an uncompressed
// minimized file ("minized.bin") or a compressed file in XZ format
// ("minimized.bin.xz"). The compressed file must use small blocks to allow
// efficient random access (see NOTES.txt for details).
class MinimizedAccessor {
public:
  explicit MinimizedAccessor(const char *filename);

  // Reads a bytes at the given file offset.
  //
  // To read multiple bytes, it may be more efficient to use ReadBytes(),
  // defined below.
  uint8_t ReadByte(int64_t offset) const;

  // Reads the bytes at the given file offsets, which must be given in
  // nondecreasing order (duplicates are allowed).
  //
  // See also XzAccessor::ReadBytes() which has the same interface.
  void ReadBytes(const int64_t *offsets, uint8_t *bytes, size_t n) const;

  // Convenience method that accepts and returns offsets and bytes in a vector.
  std::vector<uint8_t> ReadBytes(const std::vector<int64_t> &offsets) const;

private:
  std::variant<MappedMinIndex, XzAccessor> acc;
};

#endif  // ndef MINIMIZED_ACCESSOR_H_INCLUDED
