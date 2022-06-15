#!/usr/bin/env python3

# Encodes a downloadable input file in the format expected by the server.
#
# Example usage:
#
#   ./encode-input-file.py ../metadata/chunk-r22.sha256sum input/chunk-r22.sha256sum
#

from collections import OrderedDict
from codec import *
import hashlib
import sys
import zlib

if len(sys.argv) != 3:
  print('Usage: encode-input-file.py <input> <output>')
  sys.exit(1)

with open(sys.argv[1], 'rb') as f:
  data = f.read()

d = OrderedDict()
d[b'bytesize'] = EncodeInt(len(data))
d[b'sha256sum'] = hashlib.sha256(data).digest()
d[b'encoding'] = b'zlib'
d[b'encoded_data'] = zlib.compress(data)

with open(sys.argv[2], 'wb') as f:
  f.write(EncodeBytes(EncodeDict(d)))
