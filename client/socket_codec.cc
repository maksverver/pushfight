#include "socket_codec.h"

std::optional<bytes_t> DecodeBytesFromSocket(Socket &socket,size_t max_size) {
  std::vector<uint8_t> buffer(1);
  if (socket.Receive(&buffer[0], 1) != 1) {
    return {};  // EOF
  }
  size_t size = buffer[0];
  if (size > 247) {
    uint8_t buf[8];
    size_t buf_size = size - 247;  // max 8
    if (socket.ReceiveAll(buf, buf_size) != buf_size) {
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
  if (socket.ReceiveAll(data.data(), data.size()) != data.size()) {
    std::cerr << "DecodeBytesFromSocket: short read." << std::endl;
    return {};
  }
  return {std::move(data)};
}
