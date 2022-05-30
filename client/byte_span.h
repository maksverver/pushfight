#ifndef BYTE_SPAN_H_INCLUDED
#define BYTE_SPAN_H_INCLUDED

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// Readonly non-owning view of a byte array.
//
// I wrote this because I couldn't figure out how to get std::span to work :/
struct byte_span_t {
  byte_span_t() : data_(nullptr), size_(0) {}
  byte_span_t(const uint8_t *data, size_t size) : data_(data), size_(size) {}
  byte_span_t(const uint8_t *begin, const uint8_t *end) : data_(begin), size_(end - begin) {}

  byte_span_t(const std::vector<uint8_t> &b) : data_(b.data()), size_(b.size()) {}

  explicit byte_span_t(const char *s)
      : data_(reinterpret_cast<const uint8_t*>(s)), size_(strlen(s)) {
    static_assert(sizeof(*s) == sizeof(uint8_t));
  }

  static_assert(sizeof(std::string::value_type) == sizeof(uint8_t));
  explicit byte_span_t(const std::string &s)
      : data_(reinterpret_cast<const uint8_t*>(s.data())), size_(s.size()) {
    static_assert(sizeof(*s.data()) == sizeof(uint8_t));
  }

  const uint8_t *begin() const { return data_; }
  const uint8_t *end() const { return data_ + size_; }
  const uint8_t *data() const { return data_; }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  byte_span_t slice(size_t start_index) {
    return byte_span_t(data_ + start_index, size_ - start_index);
  }

  byte_span_t slice(size_t start_index, size_t end_index) {
    size_t new_size = std::min(end_index - start_index, size_ - start_index);
    return byte_span_t(data_ + start_index, new_size);
  }

  bool operator==(const byte_span_t &other) const {
    return size_ == other.size_ && memcmp(data_, other.data_, size_) == 0;
  }

  int operator<=>(const byte_span_t &other) const {
    int diff = memcmp(data_, other.data_, std::min(size_, other.size_));
    if (diff == 0) diff = (other.size_ < size_) - (size_ < other.size_);
    return diff;
  }

private:
  const uint8_t *data_;
  size_t size_;
};

inline std::ostream &operator<<(std::ostream &os, byte_span_t bytes) {
  return os.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

#endif  // ndef BYTE_SPAN_H_INCLUDED
