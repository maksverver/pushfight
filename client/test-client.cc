
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include "../flags.h"
#include "bytes.h"
#include "byte_span.h"
#include "codec.h"
#include "socket.h"
#include "socket_codec.h"

// Test client which does not hing but connect to the server and exchange
// the handshake. Used to test basic socket functionality.
int main(int argc, char *argv[]) {
  std::string host = "styx.verver.ch";
  std::string port = "7429";
  std::string user;
  std::string machine;
  std::map<std::string, Flag> flags = {
    {"host", Flag::optional(host)},
    {"port", Flag::optional(port)},
    {"user", Flag::required(user)},
    {"machine", Flag::required(machine)},
  };
  if (!ParseFlags(argc, argv, flags)) return 1;

  std::optional<Socket> socket = Socket::Connect(host.c_str(), port.c_str());
  if (!socket) {
    std::cerr << "Failed to connect!" << std::endl;
    return 1;
  }

  std::map<bytes_t, bytes_t> client_handshake;
  client_handshake[ToBytes("protocol")] = ToBytes("Push Fight 0 client");
  client_handshake[ToBytes("solver")] = ToBytes("test-client");
  client_handshake[ToBytes("user")] = ToBytes(user);
  client_handshake[ToBytes("machine")] = ToBytes(machine);

  if (!socket->SendAll(EncodeBytes(EncodeDict(client_handshake)))) {
    std::cerr << "Failed to send handshake!" << std::endl;
    return 1;
  }

  if (std::optional<bytes_t> data = DecodeBytesFromSocket(*socket); !data) {
    std::cerr << "Server refused handshake :-(" << std::endl;
  } else if (auto server_handshake = DecodeDict(*data); !server_handshake) {
    std::cerr << "Couldn't parse response dictionary!" << std::endl;
  } else if (auto it = server_handshake->find(byte_span_t("error"));
      it != server_handshake->end()) {
    std::cerr << "Handshake failed! Server returned error: \"" << it->second << "\"!" << std::endl;
  } else if (auto it = server_handshake->find(byte_span_t("protocol"));
      it == server_handshake->end() || it->second != byte_span_t("Push Fight 0 server")) {
    std::cerr << "Unsupported server protocol: "
        << (it == server_handshake->end() ? byte_span_t("unknown") : it->second) << std::endl;
  } else {
    std::cout << "Connection succesful!" << std::endl;
  }
}
