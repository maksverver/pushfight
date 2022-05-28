#ifndef CODEC_H_INCLUDED
#define CODEC_H_INCLUDED

#include <cstdint>
#include <vector>

#include "board.h"

// Encodes outcomes into bytes, with 5 ternary values per byte (i.e., 8/5 = 1.6 bits per value).
//
// The outcomes.size() must be a multiple of 5.
std::vector<uint8_t> EncodeOutcomes(const std::vector<Outcome> &outcomes);

#endif  // ndef CODEC_H_INCLUDED
