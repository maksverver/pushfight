#include "efcodec.h"

#ifdef NDEBUG
#error "Can't compile test with -DNDEBUG!"
#endif
#include <assert.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

#include "bytes.h"
#include "byte_span.h"

namespace {

std::random_device dev;
std::mt19937 rng(dev());

constexpr int64_t max_int64 = std::numeric_limits<int64_t>::max();

int64_t RandInt(int64_t min, int64_t max) {
  std::uniform_int_distribution<int64_t> dist(min, max);
  return dist(rng);
}

void Test(const std::vector<int64_t> &input) {
  std::vector<uint8_t> bytes = EncodeEF(input);
  std::vector<int64_t> output;
  if (auto res = DecodeEF(bytes); !res) {
    std::cerr << "DecodeEF() failed!\n";
    exit(1);
  } else {
    output = *res;
  }
  if (output.size() != input.size()) {
    std::cerr << "Output size is incorrect!\n"
        << "Expected: " << input.size() << "\n"
        << "Received: " << output.size() << std::endl;
    exit(1);
  }

  for (size_t i = 0; i < output.size(); ++i) {
    if (output[i] != input[i]) {
      std::cerr << "Mismatch at index " << i << "!\n"
          << "Expected: " << input[i] << "\n"
          << "Received: " << output[i] << std::endl;
      exit(1);
    }
  }

  if (input.size() > 2) {
    // Encoded size should be N*(3 + ceil(log(range / elems, 2))).
    const int elems = input.size() - 1;
    double range = (double) input.back() - input.front() + 1;
    double size_in_bits = elems * (2 + ceil(std::max(0.0, log(range / elems)) / log(2)));
    // Allow 20 extra bytes for VarInt encoding of size, tail bits, and min_value:
    assert(bytes.size() <= size_in_bits / 8 + 20);
  }
}

}  // namespace

int main() {
  assert(EFTailBits(7, 24) == 2);
  assert(EFTailBits(123, 45) == 0);
  assert(EFTailBits(1, 0) == 0);
  assert(EFTailBits(100, 100) == 0);
  assert(EFTailBits(99, 100) == 1);
  assert(EFTailBits(51, 100) == 1);
  assert(EFTailBits(50, 100) == 2);
  assert(EFTailBits(26, 100) == 2);
  assert(EFTailBits(25, 100) == 3);

  Test({});
  Test({0});
  Test({1234567890123456789});
  Test({10, 20});
  Test({100, 200, 300});
  Test({0, 1, 2, 3, 4, max_int64 - 2, max_int64 - 1, max_int64});
  Test({1, 1, 1, 2, 3, 6, 8, 8, 8, 101, 101, 101, 102, 104, 104});

  for (int test = 2; test < 60; ++test) {
    int n = RandInt(1, 10000);
    int64_t m = RandInt(1, int64_t{1} << test);
    std::vector<int64_t> v(n);
    for (size_t i = 0; i < n; ++i) {
      v[i] = RandInt(0, m - 1);
    }
    std::sort(v.begin(), v.end());
    Test(v);
  }

  {
    // Create a test stream with three parts.
    std::vector<int64_t> ints1 = {1, 2, 3};
    std::vector<int64_t> ints2 = {400, 500, 600};
    std::vector<int64_t> ints3 = {7000000, 8000000, 9000000};
    std::vector<uint8_t> part1 = EncodeEF(ints1);
    std::vector<uint8_t> part2 = EncodeEF(ints2);
    std::vector<uint8_t> part3 = EncodeEF(ints3);
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), part1.begin(), part1.end());
    combined.insert(combined.end(), part2.begin(), part2.end());
    combined.insert(combined.end(), part3.begin(), part3.end());

    // Decode multiple parts from memory.
    byte_span_t bytes(combined.data(), combined.size());
    assert(DecodeEF(&bytes).value() == ints1);
    assert(DecodeEF(&bytes).value() == ints2);
    assert(DecodeEF(&bytes).value() == ints3);
    assert(bytes.empty());
    assert(!DecodeEF(&bytes));

    // Decode multiple parts from a file.
    std::string s(reinterpret_cast<const char*>(combined.data()), combined.size());
    std::istringstream iss(s);
    assert(DecodeEF(iss).value() == ints1);
    assert(DecodeEF(iss).value() == ints2);
    assert(DecodeEF(iss).value() == ints3);
    assert(iss);
    assert(iss.get() == EOF);
    assert(iss.eof());
  }
}
