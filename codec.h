#ifndef CODEC_H_INCLUDED
#define CODEC_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <iostream>

#include "board.h"

// Encodes `outcomes` into `bytes`.
//
// This encoding packs 5 ternary values into each byte, using 8/5 = 1.6 bits
// per value. All bytes are overwritten, so it is not necessary to clear the
// bytes array before.
//
// The length of outcomes must be (at least) bytes_size * 5.
void EncodeOutcomes(const Outcome *outcomes, uint8_t *bytes, size_t bytes_size);

// Version of the above that appends bytes to the given vector.
//
// The length of outcomes must be a multiple of 5.
void EncodeOutcomes(const std::vector<Outcome> &outcomes, std::vector<uint8_t> &bytes);

// Version of the above that returns bytes in a new vector.
//
// The length of outcomes must be a multiple of 5.
std::vector<uint8_t> EncodeOutcomes(const std::vector<Outcome> &outcomes);

// Decodes `bytes` into `outcomes.
//
// The first (bytes_size * 5) elements of outcome will be overwritten, so it's
// not necessary to clear the outcome array before.
//
// The size of `outcome` must be (at least) bytes_size * 5.
void DecodeOutcomes(const uint8_t *bytes, size_t bytes_size, Outcome *outcomes);

// Version of the above that appends outcomes to the given vector.
void DecodeOutcomes(const std::vector<uint8_t> &bytes, std::vector<Outcome> &outcomes);

// Version of the above that returns outcomes in a new given vector.
std::vector<Outcome> DecodeOutcomes(const std::vector<uint8_t> &bytes);


class TernaryReader {
public:
  // Note this is the buffer size in bytes. The number of outcomes will be five
  // times this. So 1,000,000 bytes is about 6 MB total.
  static constexpr int buffer_size = 1000000;

  TernaryReader(std::istream &is) : is(is) {
    RefillBuffer();
  }

  bool HasNext() {
    return i < outcomes.size();
  }

  Outcome Next() {
    assert(i < outcomes.size());
    Outcome o = outcomes[i++];
    if (i == outcomes.size()) {
      RefillBuffer();
      i = 0;
    }
    return o;
  }

private:
  bool RefillBuffer() {
    // Try to read up to `buffer_size` bytes.
    bytes.resize(buffer_size);
    is.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    bytes.resize(is.gcount());

    if (bytes.empty()) {
      // EOF reached.
      outcomes.clear();
      assert(is.eof() && !is.bad());
      return false;
    }

    outcomes.resize(5 * bytes.size());
    DecodeOutcomes(bytes.data(), bytes.size(), outcomes.data());
    return true;
  }

  size_t i = 0;
  std::vector<Outcome> outcomes;
  std::vector<uint8_t> bytes;
  std::istream &is;
};


class BinaryWriter {
public:
  // Note this is the buffer size in bytes.
  static constexpr int buffer_size = 1000000;

  BinaryWriter(std::ostream &os) : os(os) {
    ClearBuffer();
  }

  ~BinaryWriter() {
    FlushBuffer();
  }

  void Write(bool bit) {
    bytes[i / 8] |= bit << (i % 8);
    ++i;
    if (i == buffer_size * 8) Flush();
  }

  void Flush() {
    FlushBuffer();
    ClearBuffer();
  }

private:
  void FlushBuffer() {
    assert(i % 8 == 0);
    size_t n = i / 8;
    assert(n <= bytes.size());
    if (!os.write(reinterpret_cast<char*>(bytes.data()), n)) {
      std::cerr << "BinaryWriter: write failed!" << std::endl;
      abort();
    }
  }

  void ClearBuffer() {
    i = 0;
    bytes.assign(buffer_size, uint8_t{0});
  }

  size_t i = 0;  // bit index (0 <= i < bytes.size() * 8)
  std::vector<uint8_t> bytes;
  std::ostream &os;
};

#endif  // ndef CODEC_H_INCLUDED
