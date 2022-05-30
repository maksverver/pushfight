DEFAULT_MAX_MESSAGE_SIZE = 100 << 20  # 100 MiB

class CodecError(Exception):
  pass

class DecodeError(CodecError):
  pass

class EndOfInput(DecodeError):
  def __init__(self, missing_bytes):
    self.missing_bytes = missing_bytes

  def __str__(self):
    return 'Unexpected end of input. Needed at least %s more bytes!' % self.missing_bytes

class InvalidDict(DecodeError):
  pass

class DuplicateDictKey(InvalidDict):
  def __str__(self):
    return 'Dictionary encoding contained a duplicate key.'

class MissingDictValue(InvalidDict):
  def __str__(self):
    return 'Dictionary encoding is missing a value.'

class MessageTooLarge(DecodeError):
  def __init__(self, size, max_size):
    self.size = size
    self.max_size = max_size

  def __str__(self):
    return 'Message size of %s bytes exceeds maximum of %s bytes!' % (self.size, self.max_size)


def EncodeInt(i):
  assert i >= 0
  buf = bytearray()
  while i > 0:
    buf.append(i & 0xff)
    i >>= 8
  return bytes(buf)

def EncodeBytes(data):
  buf = bytearray()
  l = len(data)
  if l < 248:
    buf.append(l)
  else:
    e = EncodeInt(l)
    buf.append(247 + len(e))
    buf.extend(e)
  buf.extend(data)
  return bytes(buf)

# Encodes a list where all values are bytestrings.
def EncodeList(l):
  return b''.join(map(EncodeBytes, l))

# Encodes a dict where keys and values are all bytestrings.
def EncodeDict(d):
  l = []
  for k, v in d.items():
    l.append(k)
    l.append(v)
  return EncodeList(l)

def DecodeInt(data):
  i = 0
  shift = 0
  for b in data:
    i |= b << shift
    shift += 8
  return i

def CheckInputSize(data, size):
  if len(data) < size:
    raise EndOfInput(size - len(data))

# Returns a pair (part, rest).
def DecodeBytes(data):
  CheckInputSize(data, 1)
  b = data[0]
  if b < 248:
    begin = 1
    end = begin + b
  else:
    k = b - 247
    CheckInputSize(data, k + 1)
    begin = k + 1
    end = begin + DecodeInt(data[1:k + 1])
  CheckInputSize(data, end)
  return (data[begin:end], data[end:])

def DecodeList(data):
  result = []
  pos = 0
  while data:
    elem, data = DecodeBytes(data)
    result.append(elem)
  return result

def DecodeDict(data):
  l = DecodeList(data)
  if len(l) % 2:
    raise MissingDictValue()
  d = {}
  for i in range(0, len(l), 2):
    k = l[i]
    v = l[i + 1]
    if k in d:
      raise DuplicateDictKey()
    d[l[i]] = v
  return d


def ReceiveAll(socket, length):
  # Repeatedly calls socket.recv() until `length` bytes have been read,
  # or throws EndOfInput if end of file is reached.'''
  buf = bytearray()
  while len(buf) < length:
    data = socket.recv(length - len(buf))
    if not data:
      raise EndOfInput(length - len(buf))
    buf.extend(data)
  return bytes(buf)


def DecodeBytesFromSocket(socket, max_message_size=DEFAULT_MAX_MESSAGE_SIZE):
  '''
    Version of DecodeByes() that reads data from a socket instead of parsing
    an existing bytestring.

    Returns None if EOF has been reached (no data read from the socket).
    Otherwise, tries to parse a message of length up to max_message_size.
  '''
  # Read length byte
  data = socket.recv(1)
  if not data:
    return None  # EOF
  length = data[0]
  if length > 247:
    length = DecodeInt(ReceiveAll(socket, length - 247))
    if length > max_message_size:
      raise MessageTooLarge(length, max_message_size)
  return ReceiveAll(socket, length)
