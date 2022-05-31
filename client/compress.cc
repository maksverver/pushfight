#include "compress.h"

#define ZLIB_CONST

#include <zlib.h>

#include <cassert>

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
