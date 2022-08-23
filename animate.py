#!/usr/bin/python3

from math import *
from PIL import Image, ImageDraw, ImageFont
import sys

H = 4
W = 8
S = 40  # scale (size of square)
P = 30  # padding

BOARD_INDEX = [
  [ -1, -1,  0,  1,  2,  3,  4, -1 ],
  [  5,  6,  7,  8,  9, 10, 11, 12 ],
  [ 13, 14, 15, 16, 17, 18, 19, 20 ],
  [ -1, 21, 22, 23, 24, 25, -1, -1 ],
]

FIELD_ROW = [
          0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,
  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3, 3,
]

FIELD_COL = [
          2,  3,  4,  5,  6,
  0,  1,  2,  3,  4,  5,  6,  7,
  0,  1,  2,  3,  4,  5,  6,  7,
      1,  2,  3,  4,  5
]

if len(sys.argv) < 3 or (sys.argv[1] == '--monochrome' and len(sys.argv) < 4):
  print('Usage: animate.py [<--monochrome>] <output.gif> <state> [<moves>...]')
  sys.exit(1)

# --monochrome enables monochrome mode, where only shades of grey are used. This
# is useful for printing still images where white (red) is to move, but not good
# for animation, since the anchor (which is white) would be invisible!
bw = sys.argv[1] == '--monochrome'
output_arg = sys.argv[1 + bw]
state_arg = sys.argv[2 + bw]
turn_args = sys.argv[3 + bw:]

if len(state_arg) != 26:
  print('Invalid state: ' + state_arg + ' (expected a 26-character string like ".OX.....oxY....Oox.....OX.")')
  sys.exit(1)

def ParseField(field):
  if len(field) != 2 or not ('a' <= field[0] <= 'h' and '1' <= field[1] <= '4'):
    print('Invalid field: ' + field)
    sys.exit(1)
  c = ord(field[0]) - ord('a')
  r = ord('4') - ord(field[1])
  assert 0 <= r < H and 0 <= c < W
  if BOARD_INDEX[r][c] < 0:
    print('Invalid field: ' + field)
    sys.exit(1)
  return (r, c)

def ParseMove(move):
  if move.count('-') != 1:
    print('Invalid move: ' + move)
    sys.exit(1)
  return tuple(map(ParseField, move.split('-')))

turns = [[ParseMove(move) for move in turn_arg.split(',')] for turn_arg in turn_args]

class Piece:
  def __init__(self, r, c, color, pusher, anchor):
    self.r = r
    self.c = c
    self.color = color
    self.pusher = pusher
    self.anchor = anchor

  def Draw(self, draw, dr, dc):
    color = [RED, BLUE][self.color]
    r = self.r + dr
    c = self.c + dc
    if self.pusher:
      draw.rectangle([P + (c + 0.1)*S, P + (r + 0.1)*S, P + (c + 0.9)*S, P + (r + 0.9)*S], fill=color, outline=BLACK)
    else:
      draw.ellipse([P + (c + 0.1)*S, P + (r + 0.1)*S, P + (c + 0.9)*S, P + (r + 0.9)*S], fill=color, outline=BLACK)
    if self.anchor:
          draw.rectangle([P + (c + 0.25)*S, P + (r + 0.45)*S, P + (c + 0.75)*S, P + (r + 0.55)*S], fill=WHITE)
          draw.rectangle([P + (c + 0.45)*S, P + (r + 0.25)*S, P + (c + 0.55)*S, P + (r + 0.75)*S], fill=WHITE)

pieces = []
for i, ch in enumerate(state_arg):
  r = FIELD_ROW[i]
  c = FIELD_COL[i]
  if ch == 'o':
    pieces.append(Piece(r, c, 0, 0, 0))
  elif ch == 'O':
    pieces.append(Piece(r, c, 0, 1, 0))
  elif ch == 'P':
    pieces.append(Piece(r, c, 0, 1, 1))
  elif ch == 'x':
    pieces.append(Piece(r, c, 1, 0, 0))
  elif ch == 'X':
    pieces.append(Piece(r, c, 1, 1, 0))
  elif ch == 'Y':
    pieces.append(Piece(r, c, 1, 1, 1))


BLACK = (  0,   0,   0)
WHITE = (255, 255, 255)
GRAY  = (128, 128, 128)
RED  =  (240,   0,   0) if not bw else (255, 255, 255)
BLUE =  (  0,   0, 240) if not bw else (0, 0, 0)
BEIGE = (209, 188, 138) if not bw else (224, 224, 224)


font = ImageFont.truetype('/usr/share/fonts/liberation/LiberationMono-Regular.ttf', 20)

def DrawBoard(draw):
  # Draw coordinates
  for c in range(W):
    s = chr(ord('a') + c)
    draw.text((P + c*S + S//2 - 2, 2), s, fill=GRAY, font=font)
    draw.text((P + c*S + S//2 - 2, 6 + P + H*S), s, fill=GRAY, font=font)
  for r in range(H):
    s = chr(ord('0') + H - r)
    draw.text((10, P + r*S + S//2 - 10), s, fill=GRAY, font=font)
    draw.text((10 + P + W*S, P + r*S + S//2 - 10), s, fill=GRAY, font=font)

  # Draw railings
  draw.rectangle([P + 2*S, P - 0.1*S, P + 7*S, P], fill=BLACK)
  draw.rectangle([P + 1*S, P + H*S, P + 6*S, P + (H + 0.1)*S], fill=BLACK)

  # Draw board
  for r in range(H):
    for c in range(W):
      i = BOARD_INDEX[r][c]
      if i >= 0:
        draw.rectangle([P + c*S, P + r*S, P + c*S + S, P + r*S + S], outline=BLACK, fill=BEIGE)

images = []
durations = []

def AddImage(duration):
  im = Image.new("RGB", (W*S + 2*P, H*S + 2*P), WHITE)
  draw = ImageDraw.Draw(im)
  DrawBoard(draw)
  for piece in pieces:
    piece.Draw(draw, 0, 0)
  images.append(im)
  durations.append(duration)

def AddAnimation(duration, frames, animated_pieces, dr, dc):
  for i in range(1, frames + 1):
    im = Image.new("RGB", (W*S + 2*P, H*S + 2*P), WHITE)
    draw = ImageDraw.Draw(im)
    DrawBoard(draw)
    for piece in pieces:
      if piece in animated_pieces:
        piece.Draw(draw, dr * i / frames, dc * i / frames)
      else:
        piece.Draw(draw, 0, 0)
    images.append(im)
    durations.append(duration)

def FindPiece(r, c):
  for p in pieces:
    if p.r == r and p.c == c:
      return p
  return None

def FindPath(r1, c1, r2, c2):
  blocked = [[False]*W for _ in range(H)]
  for piece in pieces:
    blocked[piece.r][piece.c] = True
  visited = [[None]*W for _ in range(H)]
  visited[r1][c1] = (r1, c1)
  todo = [(r1, c1)]
  for (r3, c3) in todo:
    for (r4, c4) in [(r3 - 1, c3), (r3, c3 - 1), (r3, c3 + 1), (r3 + 1, c3)]:
      if (0 <= r4 < H and 0 <= c4 < W and BOARD_INDEX[r4][c4] >= 0 and
          not blocked[r4][c4] and visited[r4][c4] is None):
        visited[r4][c4] = (r3, c3)
        if (r4, c4) == (r2, c2):
          path = [(r4, c4)]
          while (r4, c4) != (r1, c1):
            (r4, c4) = visited[r4][c4]
            path.append((r4, c4))
          path.reverse()
          return path
        todo.append((r4, c4))
  return None

# Create first frame
AddImage(1000)

if not turns:
  # Save single image
  images[0].save(output_arg)
else:
  for moves in turns:
    # Create animation
    for i, ((r1, c1), (r2, c2)) in enumerate(moves):
      # If this fails, the piece was not found on the selected location!
      piece = FindPiece(r1, c1)
      if piece is None:
        print('No piece found at', (r1, c1))
        sys.exit(1)

      if i + 1 < len(moves):
        # Regular move.
        path = FindPath(r1, c1, r2, c2)
        if path is None:
          print('No path found from', (r1, c1), 'to', (r2, c2))
          sys.exit(1)

        for j in range(len(path) - 1):
          (r1, c1), (r2, c2) = path[j:j + 2]
          AddAnimation(50, 5, [piece], r2 - r1, c2 - c1)
          piece.r = r2
          piece.c = c2

        durations[-1] = 200

      else:
        # Push move.
        for p in pieces:
          p.anchor = 0
        piece.anchor = 1
        dr = r2 - r1
        dc = c2 - c1
        moved_pieces = [piece]
        while True:
          piece = FindPiece(r1 + len(moved_pieces)*dr, c1 + len(moved_pieces)*dc)
          if piece is None:
            break
          moved_pieces.append(piece)
        AddAnimation(50, 12, moved_pieces, dr, dc)
        for piece in moved_pieces:
          piece.r += dr
          piece.c += dc

    # Show final state for 2 seconds
    AddImage(2000)

  # Save animated image
  images[0].save(output_arg, save_all=True, append_images=images[1:], loop=0, duration=durations)
