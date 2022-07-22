#include "compress.h"

#include "../bytes.h"

#define ZLIB_CONST

#include <zlib.h>

#include <cassert>
#include <optional>

bytes_t Compress(byte_span_t input_bytes) {
  bytes_t output;
  z_stream strm = {};
  int ret = deflateInit(&strm, Z_BEST_COMPRESSION);
  assert(ret == Z_OK);
  strm.next_in = input_bytes.data();
  strm.avail_in = input_bytes.size();
  do {
    unsigned char buf[16384];
    strm.next_out = buf;
    strm.avail_out = sizeof(buf);
    ret = deflate(&strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);
    int have = sizeof(buf) - strm.avail_out;
    output.insert(output.end(), &buf[0], &buf[have]);
  } while (strm.avail_out == 0);
  assert(strm.avail_in == 0);  // all input will be used
  assert(ret == Z_STREAM_END);  // stream will be complete
  deflateEnd(&strm);
  return output;
}

std::optional<bytes_t> Decompress(byte_span_t input_bytes) {
  bytes_t output;
  z_stream strm = {};
  int ret = inflateInit(&strm);
  assert(ret == Z_OK);
  strm.next_in = input_bytes.data();
  strm.avail_in = input_bytes.size();
  do {
    unsigned char buf[16384];
    strm.next_out = buf;
    strm.avail_out = sizeof(buf);
    ret = inflate(&strm, Z_NO_FLUSH);
    assert(ret != Z_STREAM_ERROR);
    if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
      // Some error in the encoded bytes.
      goto failed;
    }
    int have = sizeof(buf) - strm.avail_out;
    output.insert(output.end(), &buf[0], &buf[have]);
  } while (strm.avail_out == 0);
  if (strm.avail_in != 0) {
    // Not all input was consumed. Extra bytes after end of encoded data?
    goto failed;
  }
  if (ret != Z_STREAM_END) {
    // End of stream was not reached. Input was truncated?
    goto failed;
  }
  inflateEnd(&strm);
  return output;

failed:
  inflateEnd(&strm);
  return {};
}
