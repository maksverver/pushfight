#!/usr/bin/env python3

from datetime import datetime
import http.server
import hashlib
import socket
import socketserver
import sqlite3
import os
import sys
import time
import threading
import zlib

from codec import *

# Should be less than MAX_MESSAGE_SIZE in codec.py
MAX_CHUNK_BYTESIZE = 99 << 20  # 99 MiB

BIND_ADDR = ('', 7429)
HTTP_BIND_ADDR = ('', 7430)
WORK_EXPIRATION_SECONDS = 2*60*60  # 4 hours
MAX_CHUNKS_PER_MACHINE = 4

# For local testing:
#BIND_ADDR = ('localhost', 7429)
#WORK_EXPIRATION_SECONDS = 10
#MAX_CHUNKS_PER_MACHINE = 3

DATABASE_FILE = 'chunks.db'
assert os.path.exists(DATABASE_FILE)

UPLOAD_DIR = 'incoming'
assert os.path.isdir(UPLOAD_DIR)

INPUT_DIR = 'input'
if not os.path.isdir(INPUT_DIR):
  print('Warning: %s does not exist (or is not a directory)' % INPUT_DIR)

ARCHIVE_DIR = 'archive'
if os.path.isdir(ARCHIVE_DIR):
  archive_dirs = [
    path for path in [os.path.join(ARCHIVE_DIR, name) for name in os.listdir(ARCHIVE_DIR)]
    if os.path.isdir(path)]
  if not archive_dirs:
    print('No subdirectories in %s' % ARCHIVE_DIR)
  else:
    print('Using archived files in subdirectories:')
    for path in archive_dirs:
      print('  ', path)
else:
  print('Warning: %s does not exist (or is not a directory)' % ARCHIVE_DIR)
  archive_dirs = []


def GetSolvers(phase):
  if phase == 5: return ['solve-rN-v0.1.1']
  if phase == 6: return ['backpropagate2-v0.1.1']
  if phase == 7: return ['solve-rN-v0.1.2']
  if phase == 8: return ['backpropagate2-v0.1.3']
  if 10 <= phase <= 20: return ['solve2-v0.1.3']
  if 22 <= phase <= 24: return ['solve2-v0.1.3', 'solve2-v0.1.4', 'solve2-v0.1.5']
  if 26 <= phase <= 26: return ['solve2-v0.1.5']
  if 28 <= phase <= 40: return ['solve2-v0.1.5', 'solve2-v0.1.6']
  return []


class Server(socketserver.ThreadingMixIn, socketserver.TCPServer):
  allow_reuse_address = True

  # Bind on an IPv6 address. This should also work with IPv4 on dualstack
  # systems. To listen only on IPv4, change the value to AF_INET.
  address_family = socket.AF_INET6


def ChunkFilename(sha256sum):
  return os.path.join(UPLOAD_DIR, sha256sum.hex())


def HaveOutputFile(sha256sum):
  name = sha256sum.hex()
  filepath = os.path.join(UPLOAD_DIR, name)
  if os.path.exists(filepath):
    return True
  for dirpath in archive_dirs:
    filepath = os.path.join(dirpath, name)
    if os.path.exists(filepath) or os.path.exists(filepath + '.zst'):
      return True
  return False


def ConnectDb():
  return sqlite3.connect(DATABASE_FILE, isolation_level='EXCLUSIVE', timeout=120)


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
          elif method == 'GetCurrentPhase':
            self.handle_get_current_phase(info)
          elif method == 'DownloadInputFile':
            self.handle_download_input_file(info)
          elif method == 'GetChunks':
            self.handle_get_chunks(info)
          elif method == 'ReportChunkComplete':
            self.handle_report_chunk_complete(info)
          elif method == 'UploadChunk':
            self.handle_upload_chunk(info)
          else:
            self.send_error('Unknown method')

  def finish(self):
    print("{}: {} disconnected".format(datetime.now(), self.client_address[0]))
    self.con.close()

    # When running as a systemd service, stdout is buffered. Flush after each
    # request to make sure we can see any informational messages printed.
    # Note that stderr is still unbuffered.
    sys.stdout.flush()

  def send_response(self, response_dict):
    self.request.sendall(EncodeBytes(EncodeDict(response_dict)))

  def send_error(self, error_message):
    print('RequestHandler retuning error:', error_message)
    self.send_response({b'error': bytes(error_message, 'utf-8')})

  def handle_get_current_phase(self, info):
    with self.con:
      cur = self.con.execute('SELECT MIN(phase) FROM WorkQueue WHERE completed IS NULL')
      phase, = cur.fetchone()
      cur.close()
    print('GetCurrentPhase: returned %s' % phase)
    if phase is None:
      self.send_response({})
    else:
      self.send_response({b'phase': EncodeInt(phase)})

  def handle_download_input_file(self, info):
    filename = info[b'filename'].decode('utf-8')
    if not filename or filename.startswith('.') or filename != os.path.basename(filename):
      self.send_error('Invalid filename')
      return
    filepath = os.path.join(INPUT_DIR, filename)
    if not os.path.isfile(filepath):
      self.send_error('File not found')
      return
    print('DownloadInputFile: sending %s' % filename)
    with open(os.path.join(INPUT_DIR, filename), 'rb') as f:
      while buf := f.read(65536):
        self.request.sendall(buf)

  def handle_get_chunks(self, info):
    phase = DecodeInt(info.get(b'phase'))

    # Check solver is correct (except for phase == 999, which is used for testing).
    if phase != 999 and self.solver not in GetSolvers(phase):
      self.send_error('Invalid solver for phase')
      return

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
    if self.solver == 'solve2-v0.1.5' and not chunks:
      # Work around bug in solver which doesn't sleep between polling.
      self.con.close()
      time.sleep(10)
    print('GetChunks: returned %s chunks' % len(chunks))
    self.send_response({b'chunks': EncodeList(map(EncodeInt, chunks))})

  def handle_report_chunk_complete(self, info):
    phase = DecodeInt(info.get(b'phase'))
    chunk = DecodeInt(info.get(b'chunk'))
    bytesize = DecodeInt(info.get(b'bytesize'))
    if not (0 < bytesize <= MAX_CHUNK_BYTESIZE):
      self.send_error('Invalid chunk size: ' + str(bytesize))
      return

    sha256sum = info.get(b'sha256sum')
    if len(sha256sum) != 32:
      self.send_error('Invalid SHA256 checksum length')
      return

    now = time.time()
    with self.con:
      cur = self.con.cursor()
      cur.execute('''
          UPDATE WorkQueue
          SET completed=?, bytesize=?, sha256sum=?
          WHERE phase=? AND chunk=? AND solver=? AND user=? AND machine=? AND completed IS NULL
      ''', (now, bytesize, sha256sum, phase, chunk, self.solver, self.user, self.machine))
    if cur.rowcount != 1:
      self.send_error('Chunk not assigned')
      return
    print('ReportChunkComplete: received a report!')

    # Ask the client to upload the file if we don't already have it.
    request_upload = 0 if HaveOutputFile(sha256sum) else 1

    self.send_response({b'upload': EncodeInt(request_upload)})

  def handle_upload_chunk(self, info):
    phase = DecodeInt(info.get(b'phase'))
    chunk = DecodeInt(info.get(b'chunk'))
    encoding = info.get(b'encoding')
    encoded_data = info.get(b'encoded_data')
    if encoding != b'zlib':
      self.send_error('Unknown encoding')
      return
    if not encoded_data:
      self.send_error('Missing data')
      return

    # Note: this risks memory overflow!
    decoded_content = zlib.decompress(encoded_data)

    now = time.time()
    with self.con:
      # Fetch expected byte size and sha256sum
      cur = self.con.cursor()
      cur.execute('''
          SELECT bytesize, sha256sum FROM WorkQueue
          WHERE phase=? AND chunk=? AND solver=? AND user=? AND machine=? AND received IS NULL
      ''', (phase, chunk, self.solver, self.user, self.machine))
      row = cur.fetchone()
      if not row:
        self.send_error('Chunk not assigned')
        return

      # Check the uploaded content matches the expected size and checksum.
      bytesize, sha256sum = row
      if len(decoded_content) != bytesize:
        self.send_error('Incorrect file size')
        return
      if hashlib.sha256(decoded_content).digest() != sha256sum:
        self.send_error('Incorrect SHA256 checksum')
        return

      # Save file.
      with open(ChunkFilename(sha256sum), 'w+b') as f:
        f.write(decoded_content)

      # Store received timestamp.
      self.con.execute('''
          UPDATE WorkQueue
          SET received=?
          WHERE phase=? AND chunk=? AND received IS NULL
      ''', (now, phase, chunk))

    print('UploadChunk: received an upload!')
    self.send_response({})


class HttpServer(http.server.ThreadingHTTPServer):
  allow_reuse_address = True

  # Bind on an IPv6 address. This should also work with IPv4 on dualstack
  # systems. To listen only on IPv4, change the value to AF_INET.
  address_family = socket.AF_INET6


def ProgressBar(done, total):
  size = 100
  x = 100 * done // total
  return x*'#' + (size - x)*'.'


class HttpRequestHandler(http.server.BaseHTTPRequestHandler):
  def do_GET(self):
    con = ConnectDb()
    try:
      self.send_response(200)
      self.send_header('Content-type', 'text/plain')
      self.end_headers()
      now = time.time()
      for (
        phase, chunks_total, chunks_assigned, chunks_completed,
        difficulty_total, difficulty_assigned, difficulty_completed
      ) in con.execute('''
          SELECT
            phase,
            COUNT(*) AS total_chunks,
            SUM(completed IS NULL AND assigned IS NOT NULL AND assigned >= ?) AS assigned_chunks,
            SUM(completed IS NOT NULL) AS completed_chunks,
            SUM(difficulty) AS total_difficulty,
            SUM(difficulty*(completed IS NULL AND assigned IS NOT NULL and assigned >= ?)) AS assigned_difficulty,
            SUM(difficulty*(completed IS NOT NULL)) AS completed_difficulty
          FROM WorkQueue
          GROUP BY phase
          ORDER BY phase DESC
      ''', (now - WORK_EXPIRATION_SECONDS, now - WORK_EXPIRATION_SECONDS)):
        chunks_remaining = chunks_total - chunks_completed - chunks_assigned
        message = '''Phase %d:

  Chunks completed: %d/%d (%.2f%%)
  Chunks assigned: %d/%d (%.2f%%)
  Chunks remaining: %d/%d (%.2f%%)

  [%s]
''' % (phase,
  chunks_completed, chunks_total, 100.0*chunks_completed/chunks_total,
  chunks_assigned, chunks_total, 100.0*chunks_assigned/chunks_total,
  chunks_remaining, chunks_total, 100.0*chunks_remaining/chunks_total,
  ProgressBar(chunks_completed, chunks_total))

        if difficulty_total is not None:
          difficulty_remaining = difficulty_total - difficulty_completed - difficulty_assigned
          message += '''
  Permutations completed: %d/%d (%.2f%%)
  Permutations assigned: %d/%d (%.2f%%)
  Permutations remaining: %d/%d (%.2f%%)

  [%s]
''' % (
  difficulty_completed, difficulty_total, 100.0*difficulty_completed/difficulty_total,
  difficulty_assigned, difficulty_total, 100.0*difficulty_assigned/difficulty_total,
  difficulty_remaining, difficulty_total, 100.0*difficulty_remaining/difficulty_total,
  ProgressBar(difficulty_completed, difficulty_total))
        message += '\n'
        self.wfile.write(bytes(message, 'utf-8'))
    finally:
      con.close()


def run_http_server():
  with HttpServer(HTTP_BIND_ADDR, HttpRequestHandler) as httpd:
    httpd.serve_forever()


if __name__ == "__main__":
  if HTTP_BIND_ADDR is not None:
    threading.Thread(target=run_http_server, daemon=True).start()
  server = Server(BIND_ADDR, RequestHandler)
  server.serve_forever()
