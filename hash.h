#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include <array>
#include <cstdlib>
#include <cstdint>
#include <vector>

#include "byte_span.h"

using sha256_hash_t = std::array<uint8_t, 32>;

void ComputeSha256(const uint8_t *data, size_t size, sha256_hash_t &hash);

inline void ComputeSha256(byte_span_t data, sha256_hash_t &hash) {
  return ComputeSha256(data.data(), data.size(), hash);
}

inline sha256_hash_t ComputeSha256(byte_span_t data) {
  sha256_hash_t result;
  ComputeSha256(data.data(), data.size(), result);
  return result;
}

#endif  // ndef HASH_H_INCLUDED
