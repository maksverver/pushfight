#!/usr/bin/env python3

import socket
import socketserver
from codec import *

class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
  allow_reuse_address = True

  # Bind on an IPv6 address. This should also work with IPv4 on dualstack
  # systems. To listen only on IPv4, change the value to AF_INET.
  address_family = socket.AF_INET6


class RequestHandler(socketserver.BaseRequestHandler):
  def handle(self):
    # self.request is the TCP socket connected to the client
    print("{} connected".format(self.client_address[0]))
    while True:
      data = DecodeBytesFromSocket(self.request)
      if data is None:
        break
    print('EOF reached. Disconnecting.')

  def finish(self):
    print("{} disconnected".format(self.client_address[0]))



if __name__ == "__main__":
  addr = ('', 7429)
  # addr = ('localhost', 7429)  # for local testing
  server = Server(addr, RequestHandler)
  server.serve_forever()
