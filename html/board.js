// Defines the Push Fight board layout, as well as the representation of pieces.

'use strict';

const H = 4;
const W = 8;

const DR = Object.freeze([ -1,  0,  0,  1 ]);
const DC = Object.freeze([  0, -1, +1,  0 ]);

// Maps from (row, col) to field index (between 0 and 26, exclusive).
const FIELD_INDEX = Object.freeze([
  [ -1, -1,  0,  1,  2,  3,  4, -1 ],
  [  5,  6,  7,  8,  9, 10, 11, 12 ],
  [ 13, 14, 15, 16, 17, 18, 19, 20 ],
  [ -1, 21, 22, 23, 24, 25, -1, -1 ],
].map(Object.freeze));

function getFieldIndex(r, c) {
  return r >= 0 && r < H && c >= 0 && c < W ? FIELD_INDEX[r][c] : -1;
}

// Maps from field index to row.
const FIELD_ROW = Object.freeze([
          0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,
  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3, 3,
]);

// Maps from field index to column.
const FIELD_COL = Object.freeze([
          2,  3,  4,  5,  6,
  0,  1,  2,  3,  4,  5,  6,  7,
  0,  1,  2,  3,  4,  5,  6,  7,
      1,  2,  3,  4,  5
]);

const PieceType = {
  NONE:   0,  // empty space
  MOVER:  1,  // movable piece (circle)
  PUSHER: 2,  // pushing piece (square)
  ANCHOR: 3,  // immobile pusher
};

function getPieceType(i) {
  return i & 3;
}

function getColor(i) {
  return (i >> 2) & 1;
}

function makePiece(color, type) {
  return type | (color << 2);
}

const EMPTY_PIECES = Object.freeze([
         0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
]);

const INITIAL_PIECES = Object.freeze(getInitialPieces());

const MAX_MOVES = 2;

function getInitialPieces() {
  const a = makePiece(0, PieceType.MOVER);
  const b = makePiece(0, PieceType.PUSHER);
  const c = makePiece(1, PieceType.MOVER);
  const d = makePiece(1, PieceType.PUSHER);
  const e = makePiece(1, PieceType.ANCHOR);
  return [
          0, b, d, 0, 0,
    0, 0, 0, a, c, e, 0, 0,
    0, 0, b, a, c, 0, 0, 0,
       0, 0, b, d, 0,
  ];
}
