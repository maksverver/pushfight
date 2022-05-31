#!/usr/bin/env python3

from datetime import datetime
import socket
import socketserver
import sqlite3
import time

from codec import *

BIND_ADDR = ('', 7429)
WORK_EXPIRATION_SECONDS = 2*60*60  # 2 hour
MAX_CHUNKS_PER_MACHINE = 10

# For local testing:
#BIND_ADDR = ('localhost', 7429)
#WORK_EXPIRATION_SECONDS = 10
#MAX_CHUNKS_PER_MACHINE = 3

class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
  allow_reuse_address = True

  # Bind on an IPv6 address. This should also work with IPv4 on dualstack
  # systems. To listen only on IPv4, change the value to AF_INET.
  address_family = socket.AF_INET6


def ConnectDb():
  return sqlite3.connect('chunks.db', isolation_level='EXCLUSIVE')


class RequestHandler(socketserver.BaseRequestHandler):
  def setup(self):
    self.con = ConnectDb()

  def handle(self):
    # self.request is the TCP socket connected to the client
    print("{}: {} connected".format(datetime.now(), self.client_address[0]))

    data = DecodeBytesFromSocket(self.request)
    if data is None:
      print("Client didn't send any data!")
    else:
      info = DecodeDict(data)
      protocol = info.get(b'protocol').decode('utf-8')
      self.solver = info.get(b'solver').decode('utf-8')
      self.user = info.get(b'user').decode('utf-8')
      self.machine = info.get(b'machine').decode('utf-8')
      if protocol != 'Push Fight 0 client':
        print('Received incorrect protocol:', self.protocol)
        self.send_error('Wrong protocol')
      elif not self.solver:
        self.send_error('Missing solver')
      elif not self.user:
        self.send_error('Missing user')
      elif not self.machine:
        self.send_error('Missing machine')
      else:
        # All good. Send handshake response.
        response = {b'protocol': b'Push Fight 0 server'}
        self.request.sendall(EncodeBytes(EncodeDict(response)))

        print('Handshake completed (solver: "%s" user: "%s" machine="%s")' %
            (self.solver, self.user, self.machine))

        # Process requests.
        while True:
          data = DecodeBytesFromSocket(self.request)
          if data is None:
            break
          info = DecodeDict(data)
          method = info.get(b'method').decode('utf-8')
          if not method:
            self.send_error('Missing method')
          elif method == 'GetChunks':
            self.handle_get_chunks(info)
          elif method == 'ReportChunkComplete':
            self.handle_report_chunk_complete(info)
          else:
            self.send_error('Unknown method')

  def finish(self):
    print("{}: {} disconnected".format(datetime.now(), self.client_address[0]))
    self.con.close()

  def send_response(self, response_dict):
    self.request.sendall(EncodeBytes(EncodeDict(response_dict)))

  def send_error(self, error_message):
    print('RequestHandler retuning error:', error_message)
    self.send_response({b'error': bytes(error_message, 'utf-8')})

  def handle_get_chunks(self, info):
    phase = DecodeInt(info.get(b'phase'))
    now = time.time()
    with self.con:
      chunks = [chunk for (chunk,) in self.con.execute('''
          SELECT chunk FROM WorkQueue
          WHERE
            phase=? AND completed IS NULL AND (
              assigned IS NULL OR assigned < ? OR (user = ? AND machine = ?))
          ORDER BY assigned IS NULL, chunk
          LIMIT ?
      ''', (phase, now - WORK_EXPIRATION_SECONDS,
        self.user, self.machine, MAX_CHUNKS_PER_MACHINE))]
      # Note: this overwrites `assigned`. This means a client can keep a chunk
      # locked indefinitely by calling GetChunks periodically, but I guess we
      # have bigger problems than that if we can't trust the client to behave.
      self.con.executemany(
          'UPDATE WorkQueue SET solver=?, user=?, machine=?, assigned=? WHERE phase=? AND chunk=?',
          [(self.solver, self.user, self.machine, now, phase, chunk) for chunk in chunks])
    print('GetChunks: returned %s chunks' % len(chunks))
    self.send_response({b'chunks': EncodeList(map(EncodeInt, chunks))})

  def handle_report_chunk_complete(self, info):
    phase = DecodeInt(info.get(b'phase'))
    chunk = DecodeInt(info.get(b'chunk'))
    bytesize = DecodeInt(info.get(b'bytesize'))
    sha256sum = info.get(b'sha256sum')
    now = time.time()
    with self.con:
      cur = self.con.cursor()
      cur.execute('''
          UPDATE WorkQueue
          SET completed=?, bytesize=?, sha256sum=?
          WHERE phase=? AND chunk=? AND solver=? AND user=? AND machine=? AND completed IS NULL
      ''', (now, bytesize, sha256sum, phase, chunk, self.solver, self.user, self.machine))
    print((now, bytesize, sha256sum, phase, chunk, self.solver, self.user, self.machine))
    print(cur.rowcount)
    print('ReportChunkComplete: received a report!')
    self.send_response({b'upload': EncodeInt(0)})


if __name__ == "__main__":
  server = Server(addr, RequestHandler)
  server.serve_forever()
