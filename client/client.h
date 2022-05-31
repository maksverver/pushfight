#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <map>
#include <optional>
#include <string_view>

#include "error.h"
#include "socket.h"

class Client {
public:
  static ErrorOr<Client> Connect(
      const char *hostname, const char *portname,
      std::string_view solver, std::string_view user, std::string_view machine);

private:
  Client(Socket socket) : socket(std::move(socket)) {}

  Socket socket;
};

#endif // ndef CLIENT_H_INCLUDED
