#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include <cstdlib>
#include <optional>

#include <unistd.h>

#include "bytes.h"

class Socket {
public:
  static std::optional<Socket> Connect(const char *hostname, const char *portname);

  ~Socket() { Close(); }

  // Not copyable.
  Socket(const Socket&) = delete;
  Socket &operator=(const Socket&) = delete;

  // Movable.
  Socket(Socket&&o) { Swap(o); }
  Socket &operator=(Socket &&o) { Close(); Swap(o); return *this; }

  void Swap(Socket &o) {
    std::swap(fd, o.fd);
  }

  void Close();

  // Send all bytes and return true, or return false if an error occurred.
  bool SendAll(const uint8_t *data, size_t len);

  bool SendAll(const bytes_t &bytes) {
    return SendAll(bytes.data(), bytes.size());
  }

  // Receive anywhere between 1 and `len` bytes (blocking until at least 1 byte
  // is available.) Returns the number of bytes read, 0 on end-of-file, or
  // -1 or error.
  ssize_t Receive(uint8_t *data, size_t len);

  // Receive exactly `len` bytes, blocking until enough bytes are available if
  // necessary. This returns `len` if the buffer was filled completely, a value
  // between 0 and `len` (exclusive) if end-of-file was reached before the
  // buffer was filled, or -1 on error.
  ssize_t ReceiveAll(uint8_t *data, size_t len);

private:
  explicit Socket(int fd) : fd(fd) {}

  int fd = -1;
};

#endif  // ndef SOCKET_H_INCLUDED
