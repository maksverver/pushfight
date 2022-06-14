#ifndef SOCKET_CODEC_H_INCLUDED
#define SOCKET_CODEC_H_INCLUDED

#include <cstdint>
#include <optional>

#include "codec.h"
#include "socket.h"

constexpr size_t DEFAULT_MAX_MESSAGE_SIZE = 500 << 20;  // 500 MiB

std::optional<bytes_t> DecodeBytesFromSocket(
    Socket &socket,
    size_t max_size = DEFAULT_MAX_MESSAGE_SIZE);

#endif  // ndef SOCKET_CODEC_H_INCLUDED
