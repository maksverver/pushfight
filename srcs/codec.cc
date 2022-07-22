#include "codec.h"

#include <cassert>
#include <iostream>
#include <vector>

#include "board.h"
#include "macros.h"

void EncodeOutcomes(const Outcome *outcomes, uint8_t *bytes, size_t bytes_size) {
  while (bytes_size-- > 0) {
    *bytes++ =
        outcomes[0] +
        outcomes[1]*3 +
        outcomes[2]*3*3 +
        outcomes[3]*3*3*3 +
        outcomes[4]*3*3*3*3;
    outcomes += 5;
  }
}

void EncodeOutcomes(const std::vector<Outcome> &outcomes, std::vector<uint8_t> &bytes) {
  assert(outcomes.size() % 5 == 0);
  size_t i = bytes.size();
  size_t n = outcomes.size() / 5;
  bytes.resize(i + n);
  EncodeOutcomes(outcomes.data(), bytes.data() + i, n);
}

std::vector<uint8_t> EncodeOutcomes(const std::vector<Outcome> &outcomes) {
  std::vector<uint8_t> bytes;
  EncodeOutcomes(outcomes, bytes);
  return bytes;
}

void DecodeOutcomes(const uint8_t *bytes, size_t bytes_size, Outcome *outcomes) {
  while (bytes_size-- > 0) {
    uint8_t byte = *bytes++;
    REP(_, 5) {
      *outcomes++ = static_cast<Outcome>(byte % 3);
      byte /= 3;
    }
  }
}

void DecodeOutcomes(const std::vector<uint8_t> &bytes, std::vector<Outcome> &outcomes) {
  size_t i = outcomes.size();
  outcomes.resize(i + bytes.size() * 5);
  DecodeOutcomes(bytes.data(), bytes.size(), outcomes.data() + i);
}

std::vector<Outcome> DecodeOutcomes(const std::vector<uint8_t> &bytes) {
  std::vector<Outcome> outcomes;
  DecodeOutcomes(bytes, outcomes);
  return outcomes;
}
