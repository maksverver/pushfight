// Binary encoding/decoding of strings, integers, lists and dictionaries.
//
// The format is inspired by Bencoding, but simpler (using no ASCII delimiters).
//
// Note: the current functions make it hard to construct large nested
// data structures efficiently. (The deeper in the data structure your values
// occur the more times they need to be copied.)

#ifndef CODEC_H_INCLUDED
#define CODEC_H_INCLUDED

#include <cstdlib>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "bytes.h"
#include "byte_span.h"

// TODO: replace const bytes_t with byte_span!

uint64_t DecodeInt(const uint8_t *data, size_t len);

inline uint64_t DecodeInt(const byte_span_t &data) {
  return DecodeInt(data.data(), data.size());
}

std::optional<std::pair<byte_span_t, byte_span_t>> DecodeBytes(byte_span_t span);

std::optional<std::vector<byte_span_t>> DecodeList(byte_span_t span);

std::optional<std::map<byte_span_t, byte_span_t>> DecodeDict(byte_span_t span);

bytes_t EncodeInt(uint64_t i);

bytes_t EncodeBytes(const uint8_t *data, size_t len);

inline bytes_t EncodeBytes(const bytes_t &bytes) {
  return EncodeBytes(bytes.data(), bytes.size());
}

inline bytes_t EncodeBytes(std::string_view s) {
  static_assert(sizeof(std::string_view::value_type) == sizeof(uint8_t));
  return EncodeBytes(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

bytes_t EncodeList(const std::vector<bytes_t> &list);

bytes_t EncodeDict(const std::map<bytes_t, bytes_t> &dict) ;

static_assert(sizeof(std::string::value_type) == sizeof(bytes_t::value_type));

inline bytes_t ToBytes(std::string_view s) {
  return bytes_t(s.begin(), s.end());
}

inline byte_span_t ToByteSpan(std::string_view s) {
  return byte_span_t(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

inline std::string ToString(byte_span_t b) {
  return std::string(b.begin(), b.end());
}

#endif  // ndef CODEC_H_INCLUDED
