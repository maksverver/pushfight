#include "efcodec.h"

#include <iostream>
#include <cassert>
#include <limits>

namespace {

void AppendVarInt(bytes_t &output, int64_t value) {
  assert(value >= 0);
  do {
    uint8_t byte = value & 0x7f;
    value >>= 7;
    if (value > 0) byte |= 0x80;
    output.push_back(byte);
  } while (value > 0);
}

struct BitEncoder {
  BitEncoder(bytes_t &output) : output(output) {}

  ~BitEncoder() {
    if (pos != 0) output.push_back(byte);
  }

  void WriteBit(bool bit) {
    byte |= bit << pos++;
    if (pos == 8) {
      output.push_back(byte);
      byte = 0;
      pos = 0;
    }
  }

  // Writes the lowest num_bits (most-significant bit first).
  void WriteLowerBits(uint64_t value, int num_bits) {
    // TODO: this could be optimized a bit, by writing multiple bits at once
    // if there is space in the byte.
    //
    // NOTE: this effectly reverses the order of bits. It would have been better
    // to encode numbers in little-endian order:
    //
    //  REP(i, num_bits) WriteBit((value >> i) & 1);
    //
    // which allows reading/writing multiple bits from a byte without having
    // to reverse the bits. Unfortunately, I don't think I should change the
    // file format at this point.
    while (num_bits-- > 0) WriteBit((value >> num_bits) & 1);
  }

  void WriteUnaryNumber(uint64_t value) {
    // TODO: this could be optimized a bit, by writing multiple bits at once
    // if there is space in the byte.
    while (value-- > 0) WriteBit(0);
    WriteBit(1);
  }

private:
  bytes_t &output;
  uint8_t byte = 0;
  int pos = 0;
};

template<class ByteSource>
std::optional<int64_t> ReadVarInt(ByteSource &byte_source) {
  // Reads up to 9 bytes with 7 payload bits each for a total of 63 bits.
  int64_t value = 0;
  for (int shift = 0; shift <= 56; shift += 7) {
    auto byte = byte_source.NextByte();
    if (!byte) return {};  // end of input
    value |= int64_t{*byte & 0x7f} << shift;
    if ((*byte & 0x80) == 0) return value;
  }
  return {};
}

struct InMemoryByteSource {
  InMemoryByteSource(byte_span_t byte_span)
      : InMemoryByteSource(byte_span.data(), byte_span.size()) {}

  InMemoryByteSource(const uint8_t *data, size_t size)
      : data(data), size(size) {}

  std::optional<uint8_t> NextByte() {
    if (size == 0) return {};
    --size;
    return *data++;
  }

  const uint8_t *data;
  size_t size;
};

struct IStreamByteSource {
  IStreamByteSource(std::istream &is) : is(&is) {}

  std::optional<uint8_t> NextByte() {
    uint8_t byte;
    static_assert(sizeof(std::istream::char_type) == sizeof(byte));
    if (!is->read(reinterpret_cast<std::istream::char_type*>(&byte), 1)) {
      return {};  // end of input (or error!)
    }
    return byte;
  }

  std::istream *is;
};

template<class ByteSource>
struct BitDecoder {
  BitDecoder(ByteSource &byte_source) : byte_source(&byte_source) {}

  std::optional<bool> ReadBit() {
    if (bits == 0) {
      auto res = byte_source->NextByte();
      if (!res) return {};   // end of input
      byte = *res;
      bits = 8;
    }
    bool result = byte & 1;
    byte >>= 1;
    --bits;
    return {result};
  }

  std::optional<uint64_t> ReadLowerBits(int num_bits) {
    uint64_t value = 0;
    // TODO: this could be optimized a bit, by reading multiple bits at once
    // if there is space in the byte.
    while (num_bits-- > 0) {
      auto bit = ReadBit();
      if (!bit) return {};  // end of input
      value = (value << 1) | *bit;
    }
    return value;
  }

  std::optional<uint64_t> ReadUnaryNumber() {
    // TODO: this could be optimized a bit, by reading multiple bits at once
    // if there is space in the byte.
    uint64_t value = 0;
    for (;;) {
      auto bit = ReadBit();
      if (!bit) return {};  // end of input
      if (*bit) return value;
      ++value;
    }
  }

private:
  uint8_t byte = 0;
  int bits = 0;
  ByteSource *byte_source;
};

template<class ByteSource>
std::optional<std::vector<int64_t>> DecodeEFImpl(ByteSource &byte_source) {
  int64_t element_count;
  if (auto res = ReadVarInt(byte_source); !res) {
    return {};
  } else {
    element_count = *res;
  }
  std::vector<int64_t> result;
  if (element_count > 0) {
    result.resize(element_count);
    auto min_value = ReadVarInt(byte_source);
    if (!min_value) return {};
    result[0] = *min_value;
    if (element_count > 1) {
      int k;
      if (auto res = byte_source.NextByte(); !res) {
        return {};
      } else {
        k = *res;
      }
      if (k > 63) return {};
      BitDecoder<ByteSource> decoder(byte_source);
      for (size_t i = 1; i < element_count; ++i) {
        uint64_t delta;
        if (auto lower = decoder.ReadLowerBits(k); !lower) {
          return {};
        } else {
          delta = *lower;
        }
        if (auto upper = decoder.ReadUnaryNumber(); !upper) {
          return {};
        } else {
          delta |= *upper << k;  // possible overflow here
        }
        result[i] = result[i - 1] + delta;  // possible overflow here
      }
    }
  }
  return result;
}

}  // namespace

std::optional<std::vector<int64_t>> DecodeEF(byte_span_t *bytes) {
  InMemoryByteSource byte_source(*bytes);
  std::optional<std::vector<int64_t>> result = DecodeEFImpl<InMemoryByteSource>(byte_source);
  *bytes = byte_span_t(byte_source.data, byte_source.size);
  return result;
}

std::optional<std::vector<int64_t>> DecodeEF(std::istream &is) {
  IStreamByteSource byte_source(is);
  return DecodeEFImpl<IStreamByteSource>(byte_source);
}

void EncodeEF(const std::vector<int64_t> &sorted_ints, bytes_t &result, int k) {
  const size_t element_count = sorted_ints.size();
  AppendVarInt(result, element_count);
  if (element_count > 0) {
    const int64_t min_value = sorted_ints.front();
    assert(min_value >= 0);
    AppendVarInt(result, min_value);
    if (element_count > 1) {
      const int64_t max_value = sorted_ints.back();
      if (k < 0) k = EFTailBits(element_count, max_value);
      assert(k >= 0 && k <= 63);
      result.push_back(k);
      BitEncoder encoder(result);
      for (size_t i = 1; i < element_count; ++i) {
        assert(sorted_ints[i - 1] <= sorted_ints[i]);  // input must be sorted
        uint64_t delta = sorted_ints[i] - sorted_ints[i - 1];
        encoder.WriteLowerBits(delta, k);
        encoder.WriteUnaryNumber(delta >> k);
      }
    }
  }
}

int EFTailBits(int64_t n, int64_t m) {
  static_assert(sizeof(int64_t) == sizeof(long long));
  if (n >= m) return 0;
  int64_t x = m / n + 1;  // x = ceil((m + 1)/n); x > 1
  return 64 - __builtin_clzll(x - 1);  // ceil(log((m + 1)/n, 2))
}
