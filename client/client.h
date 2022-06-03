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
      int phase, const char *hostname, const char *portname,
      std::string_view solver, std::string_view user, std::string_view machine);

  ErrorOr<std::vector<int>> GetChunks();

  // Convencience method that calls ReportChunkComplete(), followed by
  // UploadChunk() if requested by the server.
  //
  // Returns the size of the compressed chunk that was uploaded, or 0 if the
  // chunk was not uploaded.
  ErrorOr<size_t> SendChunk(int chunk, byte_span_t content);

  ErrorOr<bool> ReportChunkComplete(
      int chunk, int64_t bytesize, sha256sum_t &hash);

  // Returns size of the upload.
  ErrorOr<size_t> UploadChunk(int chunk, byte_span_t content);

private:
  Client(int phase, Socket socket) : phase(phase), socket(std::move(socket)) {}

  int phase;
  Socket socket;
};

#endif // ndef CLIENT_H_INCLUDED
