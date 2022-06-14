#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include "../bytes.h"
#include "error.h"
#include "socket.h"

class Client {
public:
  using sha256sum_t = const std::array<uint8_t, 32>;

  static ErrorOr<Client> Connect(
      const char *hostname, const char *portname,
      std::string_view solver, std::string_view user, std::string_view machine);

  // Returns the active phase number, or an empty optional if there is none.
  ErrorOr<std::optional<int>> GetCurrentPhase();

  // Returns a list of new chunks.
  ErrorOr<std::vector<int>> GetChunks(int phase);

  // Convencience method that calls ReportChunkComplete(), followed by
  // UploadChunk() if requested by the server.
  //
  // Returns the size of the compressed chunk that was uploaded, or 0 if the
  // chunk was not uploaded.
  ErrorOr<size_t> SendChunk(
      int phase, int chunk, byte_span_t content);

  ErrorOr<bool> ReportChunkComplete(
      int phase, int chunk, int64_t bytesize, sha256sum_t &hash);

  // Returns size of the upload.
  ErrorOr<size_t> UploadChunk(int phase, int chunk, byte_span_t content);

private:
  Client(Socket socket) : socket(std::move(socket)) {}

  Socket socket;
};

#endif // ndef CLIENT_H_INCLUDED
