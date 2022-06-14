#include "client.h"

#include <cassert>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "../bytes.h"
#include "../chunks.h"
#include "../hash.h"
#include "codec.h"
#include "compress.h"
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

ErrorOr<std::optional<int>> Client::GetCurrentPhase() {
  std::map<bytes_t, bytes_t> request;
  request[ToBytes("method")] = ToBytes("GetCurrentPhase");

  if (!socket.SendAll(EncodeBytes(EncodeDict(request)))) {
    return Error("Failed to send request");
  } else if (std::optional<bytes_t> data = DecodeBytesFromSocket(socket); !data) {
    return Error("No response");
  } else if (auto response = DecodeDict(*data); !response) {
    return Error("Couldn't parse response dictionary");
  } else if (auto it = response->find(byte_span_t("error")); it != response->end()) {
    return Error("Server returned error: \"" + ToString(it->second) + "\"");
  } else if (auto it = response->find(byte_span_t("phase")); it == response->end()) {
    return {{}};  // no current phase
  } else if (auto phase = DecodeInt(it->second); !phase) {
    return Error("Couldn't parse field 'phase'.");
  } else {
    return {phase};
  }
}

ErrorOr<std::vector<int>> Client::GetChunks(int phase) {
  std::map<bytes_t, bytes_t> request;
  request[ToBytes("method")] = ToBytes("GetChunks");
  request[ToBytes("phase")] = EncodeInt(phase);

  if (!socket.SendAll(EncodeBytes(EncodeDict(request)))) {
    return Error("Failed to send request");
  } else if (std::optional<bytes_t> data = DecodeBytesFromSocket(socket); !data) {
    return Error("No response");
  } else if (auto response = DecodeDict(*data); !response) {
    return Error("Couldn't parse response dictionary");
  } else if (auto it = response->find(byte_span_t("error")); it != response->end()) {
    return Error("Server returned error: \"" + ToString(it->second) + "\"");
  } else if (auto it = response->find(byte_span_t("chunks")); it == response->end()) {
    return Error("Response is missing field 'chunks'.");
  } else if (auto parts = DecodeList(it->second); !parts) {
    return Error("Couldn't parse field 'chunks'.");
  } else {
    std::vector<int> results;
    for (const byte_span_t part : *parts) {
      uint64_t i = DecodeInt(part);
      if (i >= num_chunks) {
        return Error("Server returned invalid chunk number!");
      }
      results.push_back(i);
    }
    return results;
  }
}

ErrorOr<size_t> Client::SendChunk(int phase, int chunk, byte_span_t content) {
  sha256_hash_t hash = ComputeSha256(content);

  ErrorOr<bool> upload = ReportChunkComplete(phase, chunk, content.size(), hash);
  if (upload.IsError()) {
    return std::move(upload.Error());
  }

  if (!upload.Value()) {
    return size_t{0};
  }

  return UploadChunk(phase, chunk, content);
}

ErrorOr<bool> Client::ReportChunkComplete(
    int phase, int chunk, int64_t bytesize, sha256sum_t &hash) {
  std::map<bytes_t, bytes_t> request;
  request[ToBytes("method")] = ToBytes("ReportChunkComplete");
  request[ToBytes("phase")] = EncodeInt(phase);
  request[ToBytes("chunk")] = EncodeInt(chunk);
  request[ToBytes("bytesize")] = EncodeInt(bytesize);
  request[ToBytes("sha256sum")] = bytes_t(hash.begin(), hash.end());

  if (!socket.SendAll(EncodeBytes(EncodeDict(request)))) {
    return Error("Failed to send request");
  } else if (std::optional<bytes_t> data = DecodeBytesFromSocket(socket); !data) {
    return Error("No response");
  } else if (auto response = DecodeDict(*data); !response) {
    return Error("Couldn't parse response dictionary");
  } else if (auto it = response->find(byte_span_t("error")); it != response->end()) {
    return Error("Server returned error: \"" + ToString(it->second) + "\"");
  } else if (auto it = response->find(byte_span_t("upload")); it == response->end()) {
    return Error("Response is missing field 'upload'.");
  } else if (auto upload = DecodeInt(it->second); upload > 1) {
    return Error("Couldn't parse field 'upload'.");
  } else {
    return bool(upload);
  }
}

ErrorOr<size_t> Client::UploadChunk(
    int phase, int chunk, byte_span_t content) {
  bytes_t compressed_bytes = Compress(content);
  // Save this here because we will move out of `compressed_bytes` below.
  const size_t compressed_size = compressed_bytes.size();

  std::map<bytes_t, bytes_t> request;
  request[ToBytes("method")] = ToBytes("UploadChunk");
  request[ToBytes("phase")] = EncodeInt(phase);
  request[ToBytes("chunk")] = EncodeInt(chunk);
  request[ToBytes("encoding")] = ToBytes("zlib");
  request[ToBytes("encoded_data")] = std::move(compressed_bytes);

  if (!socket.SendAll(EncodeBytes(EncodeDict(request)))) {
    return Error("Failed to send request");
  } else if (std::optional<bytes_t> data = DecodeBytesFromSocket(socket); !data) {
    return Error("No response");
  } else if (auto response = DecodeDict(*data); !response) {
    return Error("Couldn't parse response dictionary");
  } else if (auto it = response->find(byte_span_t("error")); it != response->end()) {
    return Error("Server returned error: \"" + ToString(it->second) + "\"");
  } else {
    return compressed_size;
  }
}
