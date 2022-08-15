#!/usr/bin/env python3

# Simple HTTP server that serves results from lookup-min.

import argparse
import json
import http.server
import os
import re
import socket
import subprocess
import sys
import urllib.parse

parser = argparse.ArgumentParser(description='Serve results from lookup-min over HTTP.')
parser.add_argument('--host', default='localhost', help='Hostname to bind to')
parser.add_argument('--port', default=8003, type=int, help='Port number to bind to')
parser.add_argument('--ipv4', default=False, type=bool, action=argparse.BooleanOptionalAction,
    help='Bind to IPv4 adress instead of IPv6 address')
parser.add_argument('--lookup', default='./lookup-min', help='Path to lookup-min binary')
parser.add_argument('--minimized', default='minimized.bin', help='Path to minimized.bin or minimized.bin.xz')
args = parser.parse_args()

BIND_ADDR = (args.host, args.port)

LOOKUP_MIN_PATH = args.lookup

MINIMIZED_BIN_PATH = args.minimized

PERM_PATH = re.compile('^/perms/([.oOxXY]{26})$')


# Converts the output from lookup-min into a JSON object.
#
# Input are lines of the form:
#
#  L1 d3-b2,d2-a3,d4-e4 +8660632211
#
# Note that statuses are guaranteed to be grouped and ordered from best to
# worst outcome.
#
# Output is a JSON object of the form:
#
# {
#   "status": "W1",
#   "successors": [
#     {
#       status: "W1",
#       moves: ["a1-a2,b1-b2,c1-c2", "d1-d2", etc.]
#     },
#     {
#       status: "T",
#       moves: ["a3-a4,b3-b4,c3-c4", "d3-d4", etc.]
#     }
#     etc.
#   ]
# }
#
# Note that moves are grouped by status, but successors is a list, not an
# object, to guarantee that ordering is preserved.
def ConvertSuccessors(output):
  lines = output.splitlines()
  if not lines:
    return {'status': 'L0', 'successors': []}

  successors = []
  last_status = None
  for line in lines:
    status, moves, *rest = line.split(' ', 2)
    if status != last_status:
      last_status = status
      successors.append({'status': status, 'moves': []})
    successors[-1]['moves'].append(moves)
  return {'status': successors[0]['status'], 'successors': successors}


class HttpServer(http.server.ThreadingHTTPServer):
  allow_reuse_address = True

  address_family = socket.AF_INET if args.ipv4 else socket.AF_INET6


# Handles a request to analyze a position.
#
# Request URL must be of the form: '/perms/.OX.....oxX....Oox.....OX',
# where the second component is a 26-letter permutation string that denotes a
# started or in-progress position.
#
# On error, the response status is 400 (client error) or 500 (server error),
# and the body contains an error mesage.
#
# On success, the response status is 200, and the body is a JSON object
# as generated by Transform() above.
#
class HttpRequestHandler(http.server.BaseHTTPRequestHandler):
  def do_GET(self):
    request_url = urllib.parse.urlparse(self.path, scheme='http')

    m = PERM_PATH.match(request_url.path)
    if not m:
      self.send_error(404, 'Resource not found.')
      return
    perm = m.group(1)

    # Execute lookup-min, which does the actual work of calculating
    # successors and their statuses.
    result = subprocess.run([LOOKUP_MIN_PATH, MINIMIZED_BIN_PATH, perm],
        capture_output=True, text=True)

    if result.returncode == 2:
      # User error
      self.send_client_error(result.stderr)
      return

    if result.returncode != 0:
      # Internal or unknown error
      print('lookup-min invocation failed: perm={} returncode={} stderr={}'.format(perm, result.returncode, result.stderr), file=sys.stderr)
      self.send_server_error('Internal error. Check server logs for details.')
      return

    # Success
    assert result.returncode == 0
    assert not result.stderr
    self.send_json_response(ConvertSuccessors(result.stdout))
    return

  def send_cors_headers(self):
    self.send_header('Access-Control-Allow-Origin', '*')
    self.send_header('Access-Control-Max-Age', 3600)  # 1 hour

  def send_json_response(self, obj):
    self.send_response(200)
    self.send_cors_headers()
    self.send_header('Content-Type', 'application/json')
    self.send_header('Cache-Control', 'public, max-age=86400')  # 1 day
    self.end_headers()
    self.wfile.write(bytes(json.dumps(obj), 'utf-8'))

  def send_error(self, status, error):
    self.send_response(status)
    self.send_cors_headers()
    self.send_header('Content-Type', 'text/plain')
    self.end_headers()
    self.wfile.write(bytes(error, 'utf-8'))

  def send_client_error(self, error):
    self.send_error(400, error)

  def send_server_error(self, error):
    self.send_error(500, error)


if __name__ == "__main__":
  with HttpServer(BIND_ADDR, HttpRequestHandler) as httpd:
    print('Serving on host "%s" port %d' % BIND_ADDR)
    httpd.serve_forever()
