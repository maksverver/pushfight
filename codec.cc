#include "codec.h"

#include <cassert>
#include <iostream>
#include <vector>

#include "board.h"

// Encodes outcomes into bytes, with 5 ternary values per byte (i.e., 8/5 = 1.6 bits per value).
std::vector<uint8_t> EncodeOutcomes(const std::vector<Outcome> &outcomes) {
  assert(outcomes.size() % 5 == 0);
  std::vector<uint8_t> bytes;
  bytes.reserve(outcomes.size() * 8 / 5);
  for (size_t i = 0; i < outcomes.size(); i += 5) {
    uint8_t byte = outcomes[i] +
        outcomes[i + 1]*3 +
        outcomes[i + 2]*3*3 +
        outcomes[i + 3]*3*3*3 +
        outcomes[i + 4]*3*3*3*3;
    bytes.push_back(byte);
  }
  return bytes;
}
