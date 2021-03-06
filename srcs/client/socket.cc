#include "socket.h"

#if _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

static int CallWsaStartup() {
  WSADATA data;
  return WSAStartup(MAKEWORD(2, 2), &data);
}

static int wsa_startup_return_value = CallWsaStartup();

#else  // _WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#endif  // _WIN32

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <unistd.h>

namespace {

static_assert(sizeof(uint8_t) == sizeof(char));

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
#ifdef _WIN32
  assert(wsa_startup_return_value == 0);
#endif

  struct addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  struct addrinfo *ai = nullptr;
  if (getaddrinfo(hostname, portname, &hints, &ai) != 0) {
    std::cerr << "client: Lookup of hostname " << hostname << " failed!" << std::endl;
    return {};
  }

  for (struct addrinfo *p = ai; p != nullptr; p = p->ai_next) {
    int s = socket(p->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
      const char *family = p->ai_family == AF_INET ? "IPv4" : p->ai_family == AF_INET6 ? "IPv6" : "UNKNOWN";
      std::cerr << "client: Failed to create " << family << " socket!" << std::endl;
    } else {
      if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
        std::cerr << "client: Failed to connect to host " << hostname
            << " IP address " << AddrInfoToString(p) << " port " << portname << std::endl;
      } else {
        freeaddrinfo(ai);
        return {Socket(s)};
      }
    }
  }
  freeaddrinfo(ai);
  return {};
}

void Socket::Close() {
  if (fd >= 0) {
    // 2 == SHUT_RDWR on Linux == SD_BOTH on Windows
    shutdown(fd, 2);
    close(fd);
  }
  fd = -1;
}

bool Socket::SendAll(const uint8_t *data, size_t size) {
  while (size > 0) {
    ssize_t n = send(fd, reinterpret_cast<const char*>(data), size, 0);
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
  return recv(fd, reinterpret_cast<char*>(data), len, 0);
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
