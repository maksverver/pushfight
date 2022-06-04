#ifndef BYTES_H_INCLUDED
#define BYTES_H_INCLUDED

#include <cstdint>
#include <string>
#include <vector>

#include "byte_span.h"

using bytes_t = std::vector<uint8_t>;

inline bytes_t operator+=(bytes_t &dst, byte_span_t src) {
  dst.insert(dst.end(), src.begin(), src.end());
  return dst;
}

// Read bytes from a file. Aborts on error!
bytes_t ReadFromFile(const std::string &filename);

// Write bytes to a file. Aborts on error!
void WriteToFile(const std::string &filename, byte_span_t bytes);

// This is useful to read from e.g. stdin or generally when the input size
// is not known in advance. Aborts on error!
bytes_t ReadInput(std::istream &is);

#endif  // ndef BYTES_H_INCLUDED

