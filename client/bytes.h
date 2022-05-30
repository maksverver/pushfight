#ifndef BYTES_H_INCLUDED
#define BYTES_H_INCLUDED

#include <cstdint>
#include <vector>

#include "byte_span.h"

using bytes_t = std::vector<uint8_t>;

inline bytes_t operator+=(bytes_t &dst, byte_span_t src) {
  dst.insert(dst.end(), src.begin(), src.end());
  return dst;
}

#endif  // ndef BYTES_H_INCLUDED

