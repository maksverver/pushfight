#ifndef TERNARY_H_INCLUDED
#define TERNARY_H_INCLUDED

#include <cstdint>
#include <cstdlib>

// Decodes a ternary value from a byte at offset (i % 5).
//
// Byte must be between 0 and 243 (exclusive).
// Result will be between 0 and 3 (exclusive).
inline int DecodeTernary(uint8_t byte, size_t i) {
  switch (i % 5) {
  case 0: return byte % 3;
  case 1: return byte / 3 % 3;
  case 2: return byte / (3 * 3) % 3;
  case 3: return byte / (3 * 3 * 3) % 3;
  case 4: return byte / (3 * 3 * 3 * 3) % 3;
  default: abort();  // unreachable
  }
}

// Encodes a ternary value into a byte at offset (i % 5).
//
// Byte must be between 0 and 243 (exclusive).
// Value must be between 0 and 3 (exclusive).
// Result will be between 0 and 243 (exclusive).
inline uint8_t EncodeTernary(uint8_t byte, size_t i, int value) {
  switch (i % 5) {
  case 0: return byte + (value - DecodeTernary(byte, i));
  case 1: return byte + (value - DecodeTernary(byte, i)) * 3;
  case 2: return byte + (value - DecodeTernary(byte, i)) * 3 * 3;
  case 3: return byte + (value - DecodeTernary(byte, i)) * 3 * 3 * 3;
  case 4: return byte + (value - DecodeTernary(byte, i)) * 3 * 3 * 3 * 3;
  default: abort();  // unreachable
  }
}

#endif  // ndef TERNARY_H_INCLUDED
