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
  XzAccessor(const char *filepath);

  ~XzAccessor();

  int64_t GetUncompressedFileSize();

  // Reads `count` different bytes at the given uncompressed file offsets.
  //
  // Offsets must be between 0 and the compressed file size (exclusive),
  // and must in nondecreasing order (duplicates are allowed).
  void GetBytes(const int64_t *offsets, uint8_t *bytes, size_t count);

  // Convenience method for GetBytes() above.
  std::vector<uint8_t> GetBytes(const std::vector<int64_t> &offsets) {
    size_t count = offsets.size();
    std::vector<uint8_t> bytes(count);
    GetBytes(offsets.data(), bytes.data(), count);
    return bytes;
  }

  // Convenience method to get a single byte.
  uint8_t GetByte(int64_t offset) {
    uint8_t byte = 0;
    GetBytes(&offset, &byte, 1);
    return byte;
  }

private:
  DynMappedFile<uint8_t> mapped_file;
  lzma_stream_flags header_flags = {};
  lzma_stream_flags footer_flags = {};
  lzma_index *index = nullptr;
  int64_t block_data_start = 0;
  int64_t block_data_end = 0;

  // Decompressed block buffer. This gets reused across blocks to prevent
  // unnecessary allocations/deallocations.
  std::vector<uint8_t> buffer;
};

#endif  // ndef XZ_ACCESSOR_H_INCLUDED
