// Generates a random sample of byte offsets from the data in standard input.
//
// Phrased differently, for each byte value (0 through 255) consider the set of
// file positions where that byte appears, and output a random sample of that
// set.
//
// Output is formatted as tab-separated lines:
//
//   byte value, sample index, file position
//
// This can be used with merged.bin or minimized.bin to find examples of
// positions that are solvable in a certain amount of moves. For example, the
// offsets of byte 4 represent positions that are won-in-2.
//
// See also:
//
//   - results/merged-samples.txt
//   - results/minimized-samples.txt

#include "random.h"

#include <array>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

namespace {

// Implements the slow version of reservoir sampling (which should be good
// enough for us provided the RNG is fast enough).
//
// For a better solution, see:
// https://en.wikipedia.org/wiki/Reservoir_sampling#Optimal:_Algorithm_L
template<class T, class RNG>
struct ReservoirSampler {
  ReservoirSampler(int sample_size, RNG &rng) : rng(&rng), sample_size(sample_size) {
    samples.reserve(sample_size);
  }

  void Sample(T value) {
    if (count < sample_size) {
      samples.push_back(std::move(value));
    } else {
      std::uniform_int_distribution<int64_t> dist(0, count);
      int64_t i = dist(*rng);
      if (i < sample_size) {
        samples[i] = std::move(value);
      }
    }
    ++count;
  }

  RNG *rng;
  int sample_size;
  int64_t count = 0;
  std::vector<T> samples;
};

}  // namespace

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: sample-bytes <sample-size>" << std::endl;
    return 1;
  }

  int sample_size = 0;
  {
    std::istringstream iss(argv[1]);
    if (!(iss >> sample_size) || sample_size <= 0) {
      std::cerr << "Invalid sample size: " << sample_size << std::endl;
      return 1;
    }
  }

  std::mt19937 rng = InitializeRng();

  std::vector<ReservoirSampler<int64_t, std::mt19937>> samplers;
  samplers.reserve(256);
  for (int i = 0; i < 256; ++i) {
    samplers.emplace_back(sample_size, rng);
  }

  int64_t offset = 0;
  while (std::cin) {
    uint8_t buffer[409600];
    std::cin.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
    uint8_t *pos = buffer;
    uint8_t *end = pos + std::cin.gcount();
    while (pos != end) {
      samplers[*pos++].Sample(offset);
      ++offset;
    }
  }
  assert(std::cin.eof());

  for (int byte = 0; byte < samplers.size(); ++byte) {
    std::vector<int64_t> samples = samplers[byte].samples;
    std::sort(samples.begin(), samples.end());
    for (int i = 0; i < samples.size(); ++i) {
      std::cout << byte << '\t' << i << '\t' << samples[i] << std::endl;
    }
  }
}
