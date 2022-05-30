#!/usr/bin/env python3

import socketserver
from codec import *

class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
  allow_reuse_address = True


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
  server = Server(('localhost', 7429), RequestHandler)
  server.serve_forever()
