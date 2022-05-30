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

    protocol = solver = user = machine = None
    data = DecodeBytesFromSocket(self.request)
    if data is None:
      print("Client didn't send any data!")
    else:
      info = DecodeDict(data)
      protocol = info.get(b'protocol').decode('utf-8')
      solver = info.get(b'solver').decode('utf-8')
      user = info.get(b'user').decode('utf-8')
      machine = info.get(b'machine').decode('utf-8')
      if protocol != 'Push Fight 0 client':
        print('Wrong protocol', protocol)
      elif not solver:
        # TODO: check for known solver?
        print('Missing solver')
      elif not user:
        print('Missing user')
      elif not machine:
        print('Missing machine')
      else:
        response = {b'protocol': b'Push Fight 0 server'}
        self.request.sendall(EncodeBytes(EncodeDict(response)))

        # All good. Handle requests.
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