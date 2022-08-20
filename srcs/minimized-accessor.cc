#include "minimized-accessor.h"

#include "accessors.h"
#include "xz-accessor.h"

#include <cstdlib>
#include <filesystem>
#include <vector>

namespace {

std::variant<MappedMinIndex, XzAccessor>
OpenAccessor(const char *filename) {
  if (!std::filesystem::is_regular_file(filename)) {
    std::cerr << "File does not exit (or is not a regular file): " << filename << std::endl;
    exit(1);
  }
  if (std::filesystem::file_size(filename) == min_index_size) {
    // Open binary file with one byte postion.
    return std::variant<MappedMinIndex, XzAccessor>(std::in_place_type<MappedMinIndex>, filename);
  }
  if (XzAccessor::IsXzFile(filename)) {
    // Open XZ compressed file.
    auto var = std::variant<MappedMinIndex, XzAccessor>(std::in_place_type<XzAccessor>, filename);
    assert(std::get<XzAccessor>(var).GetUncompressedFileSize() == min_index_size);
    return var;
  }
  std::cerr << "Unkown type of file: " << filename << std::endl;
  exit(1);
}

struct ReadByteImpl {
  int64_t offset;

  uint8_t operator()(const MappedMinIndex &acc) {
    return acc[offset];
  }

  uint8_t operator()(const XzAccessor &acc) {
    return acc.ReadByte(offset);
  }
};

struct ReadBytesImpl {
  const std::vector<int64_t> &offsets;

  std::vector<uint8_t> operator()(const MappedMinIndex &acc) {
    size_t n = offsets.size();
    std::vector<uint8_t> bytes(n);
    for (size_t i = 0; i < n; ++i) {
      bytes[i] = acc[offsets[i]];
    }
    return bytes;
  }

  std::vector<uint8_t> operator()(const XzAccessor &acc) {
    return acc.ReadBytes(offsets);
  }
};

}  // namespace

MinimizedAccessor::MinimizedAccessor(const char *filename)
  : acc(OpenAccessor(filename)) {}

uint8_t MinimizedAccessor::ReadByte(int64_t offset) const {
  return std::visit(ReadByteImpl{offset}, acc);
}

std::vector<uint8_t> MinimizedAccessor::ReadBytes(const std::vector<int64_t> &offsets) const {
  return std::visit(ReadBytesImpl{offsets}, acc);
}
