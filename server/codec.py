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

# Returns a pair (part, rest).
def DecodeBytes(data):
  assert data  # should return error instead
  b = data[0]
  if b < 248:
    begin = 1
    end = begin + b
  else:
    k = b - 247
    assert k + 1 <= len(data)  # should return error instead
    begin = k + 1
    end = begin + DecodeInt(data[1:k + 1])
  assert end <= len(data)  # should return error instead
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
  assert len(l) % 2 == 0  # should return error instead.
  d = {}
  for i in range(0, len(l), 2):
    assert l[i] not in d  # should return error instead
    d[l[i]] = l[i + 1]
  return d
