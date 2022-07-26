#ifndef XZ_ACCESSOR_H_INCLUDED
#define XZ_ACCESSOR_H_INCLUDED

#include "accessors.h"

#include <lzma.h>

#include <cassert>
#include <cstdint>
#include <vector>

// Accessor that looks up bytes in a compressed .xz file.
//
// For greatest efficiency, make sure to batch lookups if there is a chance
// there are multiple references within the same block.
//
// Currently this accessor does not do ANY caching!
class XzAccessor {
public:

  // Checks if the given file refers to an XZ compressed file, by examining
  // the magic bytes at the start of the file. Returns false if the file could
  // not be opened (e.g., because it does not exist), or if the header bytes
  // are invalid. Even if this returns `true`, the file may still be corrupted
  // or incomplete.
  static bool IsXzFile(const char *filepath);

  XzAccessor(const char *filepath);

  int64_t GetUncompressedFileSize() const;

  // Reads `count` different bytes at the given uncompressed file offsets.
  //
  // Offsets must be between 0 and the compressed file size (exclusive),
  // and must in nondecreasing order (duplicates are allowed).
  void ReadBytes(const int64_t *offsets, uint8_t *bytes, size_t count) const;

  // Convenience method for ReadBytes() above.
  std::vector<uint8_t> ReadBytes(const std::vector<int64_t> &offsets) const {
    size_t count = offsets.size();
    std::vector<uint8_t> bytes(count);
    ReadBytes(offsets.data(), bytes.data(), count);
    return bytes;
  }

  // Convenience method to get a single byte.
  uint8_t ReadByte(int64_t offset) const {
    uint8_t byte = 0;
    ReadBytes(&offset, &byte, 1);
    return byte;
  }

private:
  struct IndexDeleter {
    void operator()(lzma_index *index);
  };

  DynMappedFile<uint8_t> mapped_file;
  lzma_stream_flags header_flags = {};
  lzma_stream_flags footer_flags = {};
  std::unique_ptr<lzma_index, IndexDeleter> index;
  int64_t block_data_start = 0;
  int64_t block_data_end = 0;
};

#endif  // ndef XZ_ACCESSOR_H_INCLUDED
