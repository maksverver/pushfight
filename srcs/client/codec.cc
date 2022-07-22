#include "codec.h"

#include <cassert>

namespace {

bytes_t Concatenate(const std::vector<bytes_t> &parts, size_t total_size) {
  bytes_t result;
  result.reserve(total_size);
  for (const bytes_t &part : parts) result += part;
  assert(result.size() == total_size);
  return result;
}

// Unused:
/*
size_t TotalSize(const std::vector<bytes_t> &parts) {
  size_t total_size = 0;
  for (const bytes_t &part : parts) total_size += part.size();
  return total_size;
}

bytes_t Concatenate(const std::vector<bytes_t> &parts) {
  return Concatenate(parts, TotalSize(parts));
}
*/

bool DecodeBytesImpl(
    const uint8_t *begin, const uint8_t *end,
    const uint8_t *&part_begin, const uint8_t *&part_end) {
  if (end - begin < 1) return false;
  size_t x = *begin++;
  if (x < 248) {
    if (end - begin < x) return false;
    part_begin = begin;
    part_end = begin + x;
    return true;
  }
  x -= 247;
  size_t y = DecodeInt(begin, x);
  begin += x;
  if (end - begin < y) return false;
  part_begin = begin;
  part_end = begin + y;
  return true;
}

struct PartEncoder {
  PartEncoder(int expected_parts) {
    parts.reserve(expected_parts);
  }

  void Add(const bytes_t &data) {
    const bytes_t part = EncodeBytes(data);
    total_size += part.size();
    parts.push_back(std::move(part));
  }

  bytes_t Encode() const {
    return Concatenate(parts, total_size);
  }

  size_t total_size = 0;
  std::vector<bytes_t> parts;
};

}  // namespace

uint64_t DecodeInt(const uint8_t *data, size_t len) {
  if (len > 8) len = 8;  // Can't decode more than 64 bits.
  uint64_t result = 0;
  int shift = 0;
  for (size_t i = 0; i < len; ++i) {
    result += uint64_t{data[i]} << shift;
    shift += 8;
  }
  return result;
}

std::optional<std::pair<byte_span_t, byte_span_t>> DecodeBytes(byte_span_t span) {
  const uint8_t *begin = span.data();
  const uint8_t *end = begin + span.size();
  const uint8_t *part_begin = nullptr;
  const uint8_t *part_end = nullptr;
  if (!DecodeBytesImpl(begin, end, part_begin, part_end)) {
    return {};
  }
  return {{byte_span_t(part_begin, part_end), byte_span_t(part_end, end)}};
}

std::optional<std::vector<byte_span_t>> DecodeList(byte_span_t span) {
  std::vector<byte_span_t> result;
  while (!span.empty()) {
    auto decoded = DecodeBytes(span);
    if (!decoded) return {};
    result.push_back(decoded->first);
    span = decoded->second;
  }
  return {std::move(result)};
}

std::optional<std::map<byte_span_t, byte_span_t>> DecodeDict(byte_span_t span) {
  std::optional<std::vector<byte_span_t>> decoded = DecodeList(span);
  if (!decoded) return {};
  const auto &list = *decoded;
  if (list.size() % 2 != 0) return {};  // odd number of elements
  std::map<byte_span_t, byte_span_t> result;
  for (size_t i = 0; i + 1 < list.size(); i += 2) {
    if (!result.insert({std::move(list[i]), std::move(list[i + 1])}).second) {
      return {};  // duplicate key
    }
  }
  return {std::move(result)};
}

bytes_t EncodeInt(uint64_t i) {
  bytes_t res;
  while (i > 0) {
    res.push_back(i & 0xff);
    i >>= 8;
  }
  return res;
}

bytes_t EncodeBytes(const uint8_t *data, size_t len) {
  bytes_t result;
  if (len < 248) {
    result.reserve(1 + len);
    result.push_back(len);
  } else {
    bytes_t encoded_size = EncodeInt(len);
    result.reserve(1 + encoded_size.size() + len);
    result.push_back(247 + encoded_size.size());
    result += encoded_size;
  }
  result.insert(result.end(), &data[0], &data[len]);
  return result;
}

bytes_t EncodeList(const std::vector<bytes_t> &list) {
  PartEncoder encoder = PartEncoder(list.size());
  for (const auto &elem : list) encoder.Add(elem);
  return encoder.Encode();
}

bytes_t EncodeDict(const std::map<bytes_t, bytes_t> &dict) {
  PartEncoder encoder = PartEncoder(dict.size() * 2);
  for (const auto &entry : dict) {
    encoder.Add(entry.first);
    encoder.Add(entry.second);
  }
  return encoder.Encode();
}
