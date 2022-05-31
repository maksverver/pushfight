#include <iostream>
#include <map>

#include "client.h"
#include "../flags.h"

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

  ErrorOr<Client> client_or_error =
      Client::Connect(host.c_str(), port.c_str(), "test-client", user, machine);
  if (!client_or_error) {
    std::cerr << "Failed to connect: " << client_or_error.Error().message << std::endl;
    return 1;
  }
}
