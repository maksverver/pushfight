#include "client.h"

#include "bytes.h"
#include "codec.h"
#include "socket.h"
#include "socket_codec.h"

namespace {

std::map<bytes_t, bytes_t> CreateClientHandshake(
    std::string_view solver, std::string_view user, std::string_view machine) {
  std::map<bytes_t, bytes_t> message;
  message[ToBytes("protocol")] = ToBytes("Push Fight 0 client");
  message[ToBytes("solver")] = ToBytes(solver);
  message[ToBytes("user")] = ToBytes(user);
  message[ToBytes("machine")] = ToBytes(machine);
  return message;
}

}  // namespace

ErrorOr<Client> Client::Connect(
    const char *hostname, const char *portname,
    std::string_view solver, std::string_view user, std::string_view machine) {
  std::optional<Socket> socket = Socket::Connect(hostname, portname);
  if (!socket) {
    return Error("Failed to connect");
  } else if (!socket->SendAll(EncodeBytes(EncodeDict(CreateClientHandshake(solver, user, machine))))) {
    return Error("Failed to send handshake");
  } else if (std::optional<bytes_t> data = DecodeBytesFromSocket(*socket); !data) {
    return Error("Server refused handshake");
  } else if (auto server_handshake = DecodeDict(*data); !server_handshake) {
    return Error("Couldn't parse response dictionary");
  } else if (auto it = server_handshake->find(byte_span_t("error"));
      it != server_handshake->end()) {
    return Error("Server returned error: \"" + ToString(it->second) + "\"");
  } else if (auto it = server_handshake->find(byte_span_t("protocol"));
      it == server_handshake->end() || it->second != byte_span_t("Push Fight 0 server")) {
    return Error("Unsupported server protocol: " +
        (it == server_handshake->end() ? std::string("unknown") : ToString(it->second)));
  } else {
    return Client(std::move(*socket));
  }
}
