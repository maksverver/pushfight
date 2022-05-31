#include <array>
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

  const int phase = 999;
  ErrorOr<Client> client =
      Client::Connect(host.c_str(), port.c_str(), "test-client", user, machine);
  if (!client) {
    std::cerr << "Failed to connect: " << client.Error().message << std::endl;
    return 1;
  }
  if (auto chunks = client->GetChunks(phase); !chunks) {
    std::cerr << "Failed to get chunks: " << chunks.Error().message << std::endl;
  } else {
    std::cerr << "Got chunks:\n";
    for (int i: *chunks) {
      std::cerr << i << '\n';
    }
    if (chunks->size() > 0) {
      std::array<uint8_t, 32> hash = {};
      ErrorOr<bool> upload = client->ReportChunkComplete(phase, chunks->front(), 31337, hash);
      if (!upload) {
        std::cerr << "Failed to report chunk complete: " << upload.Error().message << std::endl;
      } else {
        std::cerr << "Upload requested? " << (*upload ? "Yes!" : "No.") << std::endl;
      }
    }
  }
}
