#include "xz-accessor.h"

#include "accessors.h"

#include <lzma.h>

#include <cassert>
#include <cstdlib>
#include <iostream>

namespace {

// Allocator: null means use malloc()/free().
const lzma_allocator *allocator = nullptr;

constexpr size_t index_memory_limit = 100 << 20;  // 100 MB
constexpr size_t block_memory_limit =   1 << 20;  // 1 MB

lzma_index *CheckLzmaHeadersAndCreateIndex(
    const uint8_t *data, size_t size,
    lzma_stream_flags &header_flags,
    lzma_stream_flags &footer_flags) {
  assert(size >= 2*LZMA_STREAM_HEADER_SIZE);

  // Parse file header
  lzma_ret ret = lzma_stream_header_decode(&header_flags, data);
  assert(ret == LZMA_OK);

  // Parse file footer, expected at the end of the file. We don't support
  // multiple streams or padding at the end of the file.
  ret = lzma_stream_footer_decode(
                &footer_flags, data + size - LZMA_STREAM_HEADER_SIZE);
  assert(ret == LZMA_OK);

  ret = lzma_stream_flags_compare(&header_flags, &footer_flags);
  assert(ret == LZMA_OK);

  assert(
      footer_flags.backward_size > LZMA_BACKWARD_SIZE_MIN &&
      footer_flags.backward_size < size - 2*LZMA_STREAM_HEADER_SIZE);

  size_t in_pos = size - LZMA_STREAM_HEADER_SIZE - footer_flags.backward_size;
  lzma_index *index = nullptr;
  uint64_t memlimit = index_memory_limit;
  ret = lzma_index_buffer_decode(&index, &memlimit, allocator, data, &in_pos, size);
  if (ret == LZMA_MEMLIMIT_ERROR) {
    std::cerr << "Not enough memory for index! "
        << "Need " << memlimit << " bytes, but the limit is " << index_memory_limit << " bytes."
        << std::endl;
    exit(1);
  }
  assert(ret == LZMA_OK && index != nullptr);
  return index;
}

// Decompresses up to `output_size` bytes from a compressed block.
//
// `coffset` and `csize` give the offset and size of the block in the
// compressed file.
//
// The size of the block may be greater or smaller than `output_size`. If the
// block size is smaller, the actual number of bytes decoded is written to
// *output_size. If the block size is greater, only the first *output_size
// bytes are decoded. In both cases this function returns LMZA_OK (assuming no
// other error occurred).
lzma_ret DecompressBlockPrefix(
    const uint8_t *data, lzma_check check,
    int64_t coffset, int64_t csize,
    uint8_t *output_data, size_t *output_size) {
  // Filter list. This is initialized by lzma_block_header_decode().
  lzma_filter filters[LZMA_FILTERS_MAX + 1];

  // Figure out the block header size.
  assert(csize >= LZMA_BLOCK_HEADER_SIZE_MIN);
  uint8_t size_byte = data[coffset];
  assert(size_byte != 0);  // 0 means a stream header instead
  lzma_block block = {};
  block.version = 1;
  block.header_size = lzma_block_header_size_decode(size_byte);
  block.check = check;
  block.filters = filters;

  // Decode block header.
  lzma_ret ret = lzma_block_header_decode(&block, allocator, data + coffset + csize);
  assert(csize >= block.header_size);
  if (ret == LZMA_OK) {
    // Block header looks good. Try to decode.
    lzma_stream stream = LZMA_STREAM_INIT;
    stream.allocator = allocator;
    ret = lzma_block_decoder(&stream, &block);
    if (ret == LZMA_OK) {
      stream.next_in = data + coffset + block.header_size;
      stream.avail_in = csize - block.header_size;
      stream.next_out = output_data;
      stream.avail_out = *output_size;
      // Actual decoding loop:
      while (ret == LZMA_OK && stream.avail_out > 0) {
        ret = lzma_code(&stream, LZMA_FINISH);
      }
      if (ret == LZMA_STREAM_END) ret = LZMA_OK;
      *output_size = stream.next_out - output_data;
    }
    lzma_end(&stream);
  }

  for (size_t i = 0; i < LZMA_FILTERS_MAX; ++i) {
    assert(allocator == nullptr);  // otherwise, we should use the custom allocator
    free(filters[i].options);
  }
  return ret;
}

}  // namespace

XzAccessor::XzAccessor(const char *filepath) :
    mapped_file(filepath),
    index(
        CheckLzmaHeadersAndCreateIndex(
            mapped_file.data(), mapped_file.size(),
            header_flags, footer_flags)),
    block_data_start(LZMA_STREAM_HEADER_SIZE),
    block_data_end(mapped_file.size() - LZMA_STREAM_HEADER_SIZE - footer_flags.backward_size) {
}

XzAccessor::~XzAccessor() {
  lzma_index_end(index, allocator);
}

int64_t XzAccessor::GetUncompressedFileSize() {
  lzma_index_iter iter;
  lzma_index_iter_init(&iter, index);
  bool ret = lzma_index_iter_next(&iter, LZMA_INDEX_ITER_STREAM);
  assert(ret == false);  // true means error
  return iter.stream.uncompressed_size;
}

void XzAccessor::GetBytes(const int64_t *offsets, uint8_t *bytes, size_t count) {
  // Check that offsets nonnegative and in nondecreasing order.
  assert(offsets[0] >= 0);
  assert(std::is_sorted(&offsets[0], &offsets[count]));

  // Process offsets in order, but lookup bytes in the same block at once.
  size_t i = 0;
  while (i < count) {
    lzma_index_iter iter;
    lzma_index_iter_init(&iter, index);

    // Currently we assume the .xz file can contain blocks of variable length,
    // so we need to use lzma_index_iter_locate to find the right block. If we
    // assume blocks are all of the same size (except possibly the last), then
    // we could more efficiently index the blocks (i.e., just calculate
    // block offset / block_size).
    if (lzma_index_iter_locate(&iter, offsets[i]) == true) {
      std::cerr << "Offset out of bounds: " << offsets[i] << std::endl;
      exit(1);
    }

    const int64_t offset = iter.block.uncompressed_file_offset;
    const int64_t block_size = iter.block.uncompressed_size;

    // See how many bytes we can read from this block.
    assert(offsets[i] >= offset);
    assert(offsets[i] - offset <= block_size);
    size_t j = i + 1;
    while (j < count && offsets[j] - offset < block_size) ++j;

    // The size of the prefix of the block we need to decode.
    size_t output_size = offsets[j - 1] - offset + 1;
    assert(output_size <= block_memory_limit);
    buffer.resize(output_size);

    const int64_t coffset = iter.block.compressed_file_offset;
    const int64_t csize   = iter.block.total_size;
    assert(block_data_start <= coffset && coffset <= block_data_end);
    assert(csize <= block_data_end - coffset);
    lzma_ret ret = DecompressBlockPrefix(
        mapped_file.data(), header_flags.check,
        coffset, csize,
        buffer.data(), &output_size);
    assert(ret == LZMA_OK);
    assert(output_size == buffer.size());  // all needed data was decoded

    // Extract the requested bytes.
    while (i < j) {
      bytes[i] = buffer.at(offsets[i] - offset);
      ++i;
    }
  }
}
