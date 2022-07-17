// Defines the Push Fight board layout, as well as the representation of pieces.

'use strict';

const H = 4;
const W = 8;

const DR = Object.freeze([ -1,  0,  0,  1 ]);
const DC = Object.freeze([  0, -1, +1,  0 ]);

const FIELD_COUNT = 26;

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

const PieceType = Object.freeze({
  NONE:   0,  // empty space
  MOVER:  1,  // movable piece (circle)
  PUSHER: 2,  // pushing piece (square)
  ANCHOR: 3,  // immobile pusher
});

function getPieceType(i) {
  return i & 3;
}

function getPlayerColor(i) {
  return (i >> 2) & 1;
}

function makePiece(color, type) {
  return type | (color << 2);
}

const RED_PLAYER = 0;
const BLUE_PLAYER = 1

const NO_PIECE = 0;
const RED_MOVER = makePiece(RED_PLAYER, PieceType.MOVER);
const RED_PUSHER = makePiece(RED_PLAYER, PieceType.PUSHER);
const RED_ANCHOR = makePiece(RED_PLAYER, PieceType.ANCHOR);
const BLUE_MOVER = makePiece(BLUE_PLAYER, PieceType.MOVER);
const BLUE_PUSHER = makePiece(BLUE_PLAYER, PieceType.PUSHER);
const BLUE_ANCHOR = makePiece(BLUE_PLAYER, PieceType.ANCHOR);

const EMPTY_PIECES = Object.freeze([
         0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
]);

const INITIAL_PIECES = Object.freeze([
          0, 2, 6, 0, 0,
    0, 0, 0, 1, 5, 6, 0, 0,
    0, 0, 2, 1, 5, 0, 0, 0,
       0, 0, 2, 6, 0,
]);

const MAX_MOVES = 2;

const PiecesValidity = Object.freeze({
  INVALID:     0,
  STARTED:     1,
  IN_PROGRESS: 2,
  FINISHED:    3,
});

// Replace a piece non-destructibly.
//
// Returns a copy of `pieces` where the i-the piece has been replaced with `piece`.
function replacePiece(pieces, i, piece) {
  const result = [...pieces];
  result[i] = piece;
  return result;
}

// Returns a copy of `pieces` with the piece colors inverted
// (red pieces become blue and vice versa).
function invertColors(pieces) {
  const newPieces = [...pieces];
  for (let i = 0; i < newPieces.length; ++i) {
    const type = getPieceType(pieces[i]);
    if (type !== PieceType.NONE) {
      newPieces[i] = makePiece(1 - getPlayerColor(pieces[i]), type);
    }
  }
  return newPieces;
}

function permToPieces(perm) {
  function getPieceValue(i) {
    if (i === 0) return NO_PIECE;
    if (i === 1) return RED_MOVER;
    if (i === 2) return RED_PUSHER;
    if (i === 3) return BLUE_MOVER;
    if (i === 4) return BLUE_PUSHER;
    if (i === 5) return BLUE_ANCHOR;
  }
  return perm.map(getPieceValue);
}

function piecesToPerm(pieces, nextPlayer) {
  function getPermValue(piece) {
    const type = getPieceType(piece);
    if (type === PieceType.NONE) return 0;
    const color = getPlayerColor(piece);
    return color === nextPlayer ? type : type + 2;
  }
  return pieces.map(getPermValue);
}

// Validates the pieces represent a valid position on the board.
//
// Returns an object:
//
//  {
//    validity: int (one of the elements of PiecesValidity)
//    error: string (defined only if INVALID)
//    winner: int (defined only if FINISHED)
//    index: int (permutation index, defined only if IN_PROGRESS)
//    nextPlayer: int (defined only if STARTED or IN_PROGRESS)
//  }
function validatePieces(pieces) {

  function makeError(message) {
    return {validity: PiecesValidity.INVALID, error: message};
  }

  let redMovers = 0;
  let redPushers = 0;
  let redAnchors = 0;
  let bluePushers = 0;
  let blueMovers = 0;
  let blueAnchors = 0;
  for (let piece of pieces) {
    switch (piece) {
      case RED_MOVER:   ++redMovers;   break;
      case RED_PUSHER:  ++redPushers;  break;
      case RED_ANCHOR:  ++redAnchors;  break;
      case BLUE_MOVER:  ++blueMovers;  break;
      case BLUE_PUSHER: ++bluePushers; break;
      case BLUE_ANCHOR: ++blueAnchors; break;
      case NO_PIECE: break;
      default:
        return makeError(`Invalid piece value (${piece}).`);
    }
  }
  const redPieces = redMovers + redPushers + redAnchors;
  const bluePieces = blueMovers + bluePushers + blueAnchors;
  const totalPieces = redPieces + bluePieces;
  const totalAnchors = redAnchors + blueAnchors;
  if (redMovers > 2) {
    return makeError('Too many red movers.');
  }
  if (redPushers + redAnchors > 3) {
    return makeError('Too many red pushers.');
  }
  if (blueMovers > 2) {
    return makeError('Too many blue movers.');
  }
  if (bluePushers + blueAnchors > 3) {
    return makeError('Too many blue pushers.');
  }
  if (totalAnchors > 1) {
    return makeError('Too many anchors');
  }

  if (totalPieces === 10) {
    if (totalAnchors === 0) {
      return {
        validity: PiecesValidity.STARTED,
        nextPlayer: 0,
      };
    }

    const nextPlayer = redAnchors > 0 ? BLUE_PLAYER : RED_PLAYER;
    const perm = piecesToPerm(pieces, nextPlayer);
    return {
      validity: PiecesValidity.IN_PROGRESS,
      index: indexOfPerm(perm),
      nextPlayer: nextPlayer,
    };
  }
  if (totalPieces === 9) {
    if (totalAnchors === 0) {
      return makeError('Missing anchored piece.');
    }
    return {
      validity: PiecesValidity.FINISHED,
      winner: redPieces > bluePieces ? RED_PLAYER : BLUE_PLAYER,
    };
  }
  return makeError('Too few pieces.');
}
