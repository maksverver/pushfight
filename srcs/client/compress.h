#ifndef COMPRESS_H_INCLUDED
#define COMPRESS_H_INCLUDED

#include "../bytes.h"

#include <optional>

// Compress bytes with zlib (DEFLATE + zlib stream header).
//
// Always uses the highest compression level (9).

bytes_t Compress(byte_span_t input_bytes);

// Decompress bytes with zlib (DEFLATE + zlib stream header).
std::optional<bytes_t> Decompress(byte_span_t input_bytes);

#endif  // ndef COMPRESS_H_INCLUDED
