#!/usr/bin/env python3

from socketserver import TCPServer, ThreadingMixIn, BaseRequestHandler

class EndOfFileException(Exception):
  pass

class StreamingTokenReader:
  def __init__(self, reader):
    # reader is a function that returns a bytestring whenvever it's called,
    # returning an empty bytestring when the end of input has been reached
    self.reader = reader
    self.data = b''
    self.pos = 0

  def ReadInt(self):
    i = 0
    shift = 0
    while True:
      byte = self.NextByte()
      i += (byte & 0x7f) << shift
      if (byte & 0x80) == 0:
        return i
      shift += 7

  def ReadBytes(self):
    n = self.ReadInt()
    parts = []
    while n > 0:
      if self.pos == len(self.data):
        self.RefillBuffer()
      x = min(n, len(self.data) - self.pos)
      parts.append(self.data[self.pos:self.pos + x])
      self.pos += x
      n -= x
    return b''.join(parts)

  def RefillBuffer(self):
    self.pos = 0
    self.data = self.reader()
    if not self.data:
      raise EndOfFileException()

  def NextByte(self):
    if self.pos == len(self.data):
      self.RefillBuffer()
    b = self.data[self.pos]
    self.pos += 1
    return b


class BufferedTokenWriter:
  def __init__(self):
    self.data = bytearray()

  def WriteInt(self, i):
    assert i >= 0
    while True:
      v = i & 0x7f
      i >>= 7
      if i == 0:
        self.data.append(v)
        break
      self.data.append(v | 0x80)

  def WriteBytes(self, data):
    self.WriteInt(len(data))
    self.data.extend(data)

  def Flush(self):
    result = bytes(self.data)
    self.data = bytearray()
    return result

class MessageWriter:
  def __init__(self, token_writer):
    self.token_writer = token_writer

  def WriteUInt(self, field, u, default=0):
    assert field > 0
    if u != default:
      self.token_writer.WriteInt(4*field + 1)
      self.token_writer.WriteInt(u)

  def WriteBytes(self, field, b, default=b''):
    assert field > 0
    if b != default:
      self.token_writer.WriteInt(4*field + 2)
      self.token_writer.WriteBytes(b)

  def WriteString(self, field, s, default=''):
    assert field > 0
    if s != default:
      self.token_writer.WriteInt(4*field + 2)
      self.token_writer.WriteBytes(bytes(s, 'utf-8'))

  def EndMessage(self):
    self.token_writer.WriteInt(0)

  def WriteMessage(self, field, msg):
    assert field > 0
    if msg is not None:
      self.token_writer.WriteInt(4*field + 0)
      msg.Write(self)


class MessageParser:
  def __init__(self, tokens):
    self.tokens = tokens
    self.last_type = None

  def NextField(self):
    assert self.last_type is None
    tag = self.tokens.ReadInt()
    field = tag >> 2
    if field == 0:
      assert (tag & 3) == 0
      return 0
    self.last_type = tag & 3
    return field

  def NextType(self):
    result = self.last_type
    assert result is not None
    self.last_type = None
    return result

  # A nicer design would use a separate codec to construct instances. This would
  # also enable builders for immutable objects etc. Something like:
  #
  #  ParseMessage(self, codec):
  #    builder = codec.Start()
  #    builder.Read(self)
  #    return builder.Finish()
  #
  # However, this would lead to a bit of extra boilerplate. So I'll keep things
  # simple and just use mutable objects instead.
  def ParseMessage(self, msg_type, default=None):
    if self.NextType() != 0:
      return default
    msg = msg_type()
    msg.Read(self)
    return msg

  def ParseUInt(self, default=0):
    if self.NextType() != 1:
      return default
    return self.tokens.ReadInt()

  def ParseBytes(self, default=b''):
    if self.NextType() != 2:
      return default
    return self.tokens.ReadBytes()

  def ParseString(self, default=''):
    if self.NextType() != 2:
      return default
    return self.tokens.ReadBytes().decode('utf-8')

  def Skip(self):
    if self.last_type == 0:
      #
      # TODO: recursively skip past messages...
      #
      assert False
      pass
    elif self.last_type == 1:
      self.tokens.ReadInt()
    elif self.last_type == 2:
      self.tokens.ReadBytes()
    else:
      # Invalid type 3 used
      assert False

# Reader that "reads" from a bytestring or bytearray. Mostly for testing.
class BytesReader:
  def __init__(self, data, chunk_size):
    self.data = data
    self.chunk_size = chunk_size
    self.pos = 0

  def NextBytes(self):
    end = self.pos + min(self.chunk_size, len(self.data) - self.pos)
    data = self.data[self.pos:end]
    self.pos = end
    return data


class ClientInfo:

  def __init__(self):
    self.solver_id = ''
    self.user_id = ''
    self.machine_id = ''

  def __str__(self):
    return 'ClientInfo{solver_id=%s user_id=%s machine_id=%s}' % (self.solver_id, self.user_id, self.machine_id)

  def Read(msg, parser):
    while field := parser.NextField():
      if field == 1:
        msg.solver_id = parser.ParseString('')
      elif field == 2:
        msg.user_id = parser.ParseString('')
      elif field == 3:
        msg.machine_id = parser.ParseString('')
      else:
        parser.Skip()

  def Write(msg, writer):
    writer.WriteString(1, msg.solver_id, '')
    writer.WriteString(2, msg.user_id, '')
    writer.WriteString(3, msg.machine_id, '')
    writer.EndMessage()



class GetChunksRequest:
  def __init__(self):
    self.client = None
    self.phase = 0

  def __str__(self):
    return 'GetChunksRequest{client=%s phase=%s}' % (self.client, self.phase)

  def Read(self, parser):
    while field := parser.NextField():
      if field == 1:
        self.client = parser.ParseMessage(ClientInfo)
      if field == 2:
        self.phase = parser.ParseUInt()

  def Write(self, writer):
    writer.WriteMessage(1, self.client)
    writer.WriteUInt(2, self.phase)
    writer.EndMessage()


def EncodeMessage(msg):
  w = BufferedTokenWriter()
  msg.Write(MessageWriter(w))
  return w.Flush()

def TestEncode():
  ci = ClientInfo()
  ci.solver_id = 'solver'
  ci.user_id = 'user'
  ci.machine_id = 'machine'
  req = GetChunksRequest()
  req.client = ci
  req.phase = 42
  return EncodeMessage(req)

data = TestEncode()
print(data)

msg = GetChunksRequest()
msg.Read(MessageParser(StreamingTokenReader(BytesReader(data, 1024).NextBytes)))
print(msg)
