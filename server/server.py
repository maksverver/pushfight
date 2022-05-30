#!/usr/bin/env python3

import socketserver
from codec import *

MAX_MESSAGE_SIZE = 100 << 20  # 100 MiB


def DecodeBytesFromSocket(s):
  # Read length byte
  data = s.recv(1)
  if not data:
    return None  # EOF
  length = data[0]
  if length > 247:
    # Read additional length bytes
    data = s.recv(length - 247)
    assert len(data) == length - 247  # short read; should return error instead
    length = DecodeInt(data)
    assert length <= MAX_MESSAGE_SIZE  # message too large

  # Read fixed message size
  buf = bytearray()
  while len(buf) < length:
    data = s.recv(length - len(buf))
    assert data  # short read; should return error instead
    buf.extend(data)
  assert len(buf) == length
  return bytes(buf)


class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
  allow_reuse_address = True


class RequestHandler(socketserver.BaseRequestHandler):
  def handle(self):
    # self.request is the TCP socket connected to the client
    print("{} connected".format(self.client_address[0]))
    while True:
      data = DecodeBytesFromSocket(self.request)
      print('here', data)
      # TODO

  def finish(self):
    print("{} disconnected".format(self.client_address[0]))



if __name__ == "__main__":
  server = Server(('localhost', 7429), RequestHandler)
  server.serve_forever()
