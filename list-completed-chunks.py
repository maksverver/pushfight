#!/usr/bin/python3

# Lists the completed chunks in the binary output file produced by
# backpropagate-losses.
#
# The file starts with a header of 4096 bytes, of which the first 7429 bits mark
# the chunks that have been completed. The remaining bits are supposed to be
# zero (which the tool also checks).
#
# Completed chunk numbers are listed line-by-line. Each line contains either
# a single chunk or a range with the second endpoint exclusive. For example:
#
#   100       (1 chunk with index 100)
#   200-204   (4 chunk with indices 200, 201, 202, 203)
#

import sys

if len(sys.argv) != 2:
  print('Usage: %s <filename>' % sys.argv[0])
  sys.exit(0)

num_chunks = 7429
size_bytes = 4096
size_bits = size_bytes * 8

with open(sys.argv[1], mode='rb') as file:
  data = file.read(size_bytes)

if len(data) != size_bytes:
  print('Short read from input! Expected %d bytes, read %d.' % (size_bytes, len(data)))
  sys.exit(1)

def GetBit(i):
  return (data[i // 8] > (i % 8)) & 1

i = 0
while i < size_bits:
  while i < size_bits and GetBit(i) == 0:
    i += 1
  if i == size_bits:
    break
  if i >= num_chunks:
    print('WARNING: detected unexpected 1 bit at index', i)
    break
  j = i + 1
  while j < num_chunks and GetBit(j) == 1:
    j += 1
  if j - i == 1:
    print('%d' % (i,))  # single chunk
  if j - i > 1:
    print('%d-%d' % (i, j))  # range from i to j (exclusive)
  i = j
