#!/usr/bin/python3

H = 4
W = 8

BOARD_INDEX = [
  [ -1, -1,  0,  1,  2,  3,  4, -1 ],
  [  5,  6,  7,  8,  9, 10, 11, 12 ],
  [ 13, 14, 15, 16, 17, 18, 19, 20 ],
  [ -1, 21, 22, 23, 24, 25, -1, -1 ],
]

neighbors = [None]*26
for r1 in range(H):
  for c1 in range(W):
    i = BOARD_INDEX[r1][c1]
    if i < 0: continue
    assert neighbors[i] is None
    neighbors[i] = []
    for (r2, c2, r3, c3) in [(r1 - 1, c1, r1 + 1, c1), (r1, c1 - 1, r1, c1 + 1)]:
      j = BOARD_INDEX[r2][c2] if 0 <= r2 < H and 0 <= c2 < W else -1
      k = BOARD_INDEX[r3][c3] if 0 <= r3 < H and 0 <= c3 < W else -1
      if j >= 0 and k >= 0:
        neighbors[i].append(j)
        neighbors[i].append(k)

for i, ns in enumerate(neighbors):
  print('  {' + ', '.join(' %2d'%i for i in ns + [-1]*(6 - len(ns)))  + ' },  // %2d'%i)
