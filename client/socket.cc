#include "socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

namespace {

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

}  // namespace

std::optional<Socket> Socket::Connect(const char *hostname, const char *portname) {
  struct addrinfo *ai = nullptr;
  if (getaddrinfo(hostname, portname, nullptr, &ai) != 0) {
    std::cerr << "client: Lookup of hostname " << hostname << " failed!" << std::endl;
    return {};
  }

  for (struct addrinfo *p = ai; p != nullptr; p = p->ai_next) {
    int s = socket(ai->ai_family, SOCK_STREAM, 0);
    if (s == -1) {
      const char *family = ai->ai_family == AF_INET ? "IPv4" : ai->ai_family == AF_INET6 ? "IPv6" : "UNKNOWN";
      std::cerr << "client: Failed to create " << family << " socket!" << std::endl;
    } else {
      if (connect(s, ai->ai_addr, ai->ai_addrlen) == -1) {
        std::cerr << "client: Failed to connect to host " << hostname
            << " IP address " << AddrInfoToString(ai) << " port " << portname << std::endl;
      } else {
        freeaddrinfo(ai);
        return {Socket(s)};
      }
    }
  }
  freeaddrinfo(ai);
  return {};
}

bool Socket::SendAll(const uint8_t *data, size_t size) {
  while (size > 0) {
    ssize_t n = send(fd, data, size, 0);
    if (n <= 0) {
      std::cerr << "Failed to sent data over socket!" << std::endl;
      return false;
    }
    data += n;
    size -= n;
  }
  return true;
}

ssize_t Socket::Receive(uint8_t *data, size_t len) {
  return recv(fd, data, len, 0);
}

ssize_t Socket::ReceiveAll(uint8_t *data, size_t len) {
  ssize_t pos = 0;
  while (pos < len) {
    ssize_t n = Receive(data + pos, len - pos);
    if (n < 0) return n;  // error
    if (n == 0) break;  // EOF
    assert(n <= len - pos);
    pos += n;
  }
  return pos;
}
