#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <vector>

using bytes_t = std::vector<uint8_t>;

// Readonly non-owning view of a byte array.
//
// I wrote this because I couldn't figure out how to get std::span to work :/
struct byte_span_t {
  byte_span_t(const uint8_t *data, size_t size) : data_(data), size_(size) {}
  byte_span_t(const uint8_t *begin, const uint8_t *end) : data_(begin), size_(end - begin) {}
  template<class T> byte_span_t(const T &v) : data_(v.data()), size_(v.size()) {}

  const uint8_t *begin() const { return data_; }
  const uint8_t *end() const { return data_ + size_; }
  const uint8_t *data() const { return data_; }
  size_t size() const { return size_; }

  byte_span_t slice(size_t start_index) {
    return byte_span_t(data_ + start_index, size_ - start_index);
  }

  byte_span_t slice(size_t start_index, size_t end_index) {
    size_t new_size = std::min(end_index - start_index, size_ - start_index);
    return byte_span_t(data_ + start_index, new_size);
  }

private:
  const uint8_t *data_;
  size_t size_;
};

// TODO: a lot of these should probably take a byte_span_t instead of const bytes_t.
bytes_t operator+=(bytes_t &dst, const bytes_t &src) {
  dst.insert(dst.end(), src.begin(), src.end());
  return dst;
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

bytes_t EncodeBytes(const bytes_t &bytes) {
  return EncodeBytes(bytes.data(), bytes.size());
}

bytes_t EncodeBytes(std::string_view s) {
  static_assert(sizeof(std::string_view::value_type) == sizeof(uint8_t));
  return EncodeBytes(reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

size_t TotalSize(const std::vector<bytes_t> &parts) {
  size_t total_size = 0;
  for (const bytes_t &part : parts) total_size += part.size();
  return total_size;
}

bytes_t Concatenate(const std::vector<bytes_t> &parts, size_t total_size) {
  bytes_t result;
  result.reserve(total_size);
  for (const bytes_t &part : parts) result += part;
  assert(result.size() == total_size);
  return result;
}

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

uint64_t DecodeInt(const bytes_t &data) {
  return DecodeInt(data.data(), data.size());
}

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

// TODO:
/*
def DecodeList(data):
  result = []
  pos = 0
  while data:
    elem, data = DecodeBytes(data)
    result.append(elem)
  return result

def DecodeDict(data):
  l = DecodeList(data)
  if len(l) % 2:
    raise MissingDictValue()
  d = {}
  for i in range(0, len(l), 2):
    k = l[i]
    v = l[i + 1]
    if k in d:
      raise DuplicateDictKey()
    d[l[i]] = v
  return d
*/

bytes_t Concatenate(const std::vector<bytes_t> &parts) {
  return Concatenate(parts, TotalSize(parts));
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

static_assert(sizeof(std::string::value_type) == sizeof(bytes_t::value_type));

bytes_t ToBytes(const std::string &s) {
  return bytes_t(s.begin(), s.end());
}

std::string ToString(const bytes_t &b) {
  return std::string(b.begin(), b.end());
}

std::string AddrInfoToString(const addrinfo *ai) {
  if (ai->ai_family == AF_INET) {
      char buf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((struct sockaddr_in *)ai->ai_addr)->sin_addr, buf, sizeof buf);
      return std::string(buf);
  }
  if (ai->ai_family == AF_INET6) {
      char buf[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr, buf, sizeof buf);
      return std::string(buf);
  }
  return "UNKNOWN";
}

int Connect(const char *hostname, const char *portname) {
  struct addrinfo *ai = nullptr;
  if (getaddrinfo(hostname, portname, nullptr, &ai) != 0) {
    std::cerr << "client: Hostname lookuped failed!" << std::endl;
    return -1;
  }

  for (struct addrinfo *p = ai; p != nullptr; p = p->ai_next) {
    int s = socket(ai->ai_family, SOCK_STREAM, 0);
    if (s == -1) {
      const char *family = ai->ai_family == AF_INET ? "IPv4" : ai->ai_family == AF_INET6 ? "IPv6" : "UNKNOWN";
      std::cerr << "client: Failed to create " << family << " socket!" << std::endl;
    } else {
      if (connect(s, ai->ai_addr, ai->ai_addrlen) == -1) {
        std::cerr << "client: Failed to connect to host " << hostname << " IP address " << AddrInfoToString(ai) << std::endl;
      } else {
        std::cerr << "client: Connected to host " << hostname << " IP address " << AddrInfoToString(ai) << std::endl;
        freeaddrinfo(ai);
        return s;
      }
    }
  }
  freeaddrinfo(ai);
  return -1;
}

bool Send(int socket, const bytes_t &data) {
  if (send(socket, data.data(), data.size(), 0) != data.size()) {
    std::cerr << "Failed to sent data over socket!" << std::endl;
    return false;
  }
  return true;
}

ssize_t Receive(int socket, uint8_t *data, size_t len) {
  return recv(socket, data, len, 0);
}

ssize_t ReceiveAll(int socket, uint8_t *data, size_t len) {
  ssize_t pos = 0;
  while (pos < len) {
    ssize_t n = Receive(socket, data + pos, len - pos);
    if (n < 0) return n;  // error
    if (n == 0) break;  // EOF
    assert(n <= len - pos);
    pos += n;
  }
  return pos;
}

constexpr size_t DEFAULT_MAX_MESSAGE_SIZE = 100 << 20;  // 100 MiB

std::optional<bytes_t> DecodeBytesFromSocket(
    int socket, size_t max_size = DEFAULT_MAX_MESSAGE_SIZE) {
  std::vector<uint8_t> buffer(1);
  if (Receive(socket, &buffer[0], 1) != 1) {
    return {};  // EOF
  }
  size_t size = buffer[0];
  if (size > 247) {
    uint8_t buf[8];
    size_t buf_size = size - 247;  // max 8
    if (ReceiveAll(socket, buf, buf_size) != buf_size) {
      std::cerr << "DecodeBytesFromSocket: short read." << std::endl;
      return {};
    }
    size = DecodeInt(buf, buf_size);
    if (size > max_size) {
      std::cerr << "DecodeBytesFromSocket: message size of " << size
           << " bytes exceeds limit of " << max_size << " bytes!" << std::endl;
      return {};
    }
  }
  bytes_t data;
  data.resize(size);
  if (ReceiveAll(socket, data.data(), data.size()) != data.size()) {
    std::cerr << "DecodeBytesFromSocket: short read." << std::endl;
    return {};
  }
  return {std::move(data)};
}


int main() {
  //const char *hostname = "styx.verver.ch";
  int socket = Connect("localhost", "7429");
  if (socket != -1) {

    std::map<bytes_t, bytes_t> first_message;
    first_message[ToBytes("protocol")] = ToBytes("Push Fight 0 client");
    first_message[ToBytes("solver")] = ToBytes("some-solver");
    first_message[ToBytes("user")] = ToBytes("maks");
    first_message[ToBytes("machine")] = ToBytes("styx");

    if (Send(socket, EncodeBytes(EncodeDict(first_message)))) {
      std::optional<bytes_t> data = DecodeBytesFromSocket(socket);
      if (!data) {
        std::cerr << "I said hello but received no response :-(" << std::endl;
      } else {
        // TODO: decode dict etc.
        std::string s((const char*)data->data(), data->size());
        std::cout << s << std::endl;
      }
    }

    //send(s, const void *msg, int len, int flags);
    close(socket);
  }
}
