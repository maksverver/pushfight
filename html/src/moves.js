import {
  H, W, FIELD_ROW, FIELD_COL, DR, DC, getFieldIndex,
  PieceType, getPieceType, getPlayerColor, makePiece,
} from './board.js';


export function* generateMoveDestinations(pieces, initialField) {
  // Breadth-first search for spaces reachable from field i without going through pieces.
  const visited = new Set();
  visited.add(initialField);
  const todo = [initialField];
  for (let i = 0; i < todo.length; ++i) {
    const i1 = todo[i];
    const r1 = FIELD_ROW[i1];
    const c1 = FIELD_COL[i1];
    for (let d = 0; d < 4; ++d) {
      const r2 = r1 + DR[d];
      const c2 = c1 + DC[d];
      const i2 = getFieldIndex(r2, c2);
      if (i2 >= 0 && getPieceType(pieces[i2]) === PieceType.NONE) {
        if (!visited.has(i2)) {
          visited.add(i2);
          todo.push(i2);
          yield i2;
        }
      }
    }
  }
}

function canPush(pieces, i, d) {
  let r = FIELD_ROW[i];
  let c = FIELD_COL[i];
  const dr = DR[d];
  const dc = DC[d];
  r += dr;
  c += dc;
  i = getFieldIndex(r, c);
  if (i === -1 || pieces[i] === 0) {
    // Must push at least one piece.
    return false;
  }
  while (i !== -1 && pieces[i] !== 0) {
    if (getPieceType(pieces[i]) === PieceType.ANCHOR) {
      // Cannot push anchored piece.
      return false;
    }
    r += dr;
    c += dc;
    if (r < 0 || r >= H) {
      // Canot push pieces past the guard rail at the top/bottom of the board.
      return false;
    }
    i = getFieldIndex(r, c);
  }
  return true;
}

function* findPushDirections(pieces, i) {
  for (let d = 0; d < 4; ++d) {
    if (canPush(pieces, i, d)) {
      yield d;
    }
  }
}

export function* findPushDestinations(pieces, i) {
  const r = FIELD_ROW[i];
  const c = FIELD_COL[i];
  for (const d of findPushDirections(pieces, i)) {
    yield getFieldIndex(r + DR[d], c + DC[d]);
  }
}

export function* generateMoves(pieces, player) {
  for (let i = 0; i < pieces.length; ++i) {
    if (getPieceType(pieces[i]) !== PieceType.NONE && getPlayerColor(pieces[i]) === player) {
      // Sort destinations to facilitate debugging.
      const js = Array.from(generateMoveDestinations(pieces, i)).sort((a, b) => a - b);
      for (const j of js) {
        yield [i, j];
      }
    }
  }
}

export function isValidMove(pieces, player, i, j) {
  for (const [k, l] of generateMoves(pieces, player)) {
    if (i === k && j === l) return true;
  }
  return false;
}

export function* generatePushes(pieces, player) {
  for (let i = 0; i < pieces.length; ++i) {
    if (getPieceType(pieces[i]) === PieceType.PUSHER && getPlayerColor(pieces[i]) === player) {
      for (const d of findPushDirections(pieces, i)) {
        yield [i, getFieldIndex(FIELD_ROW[i] + DR[d], FIELD_COL[i] + DC[d])];
      }
    }
  }
}

export function isValidPush(pieces, player, i, j) {
  for (const [k, l] of generatePushes(pieces, player)) {
    if (i === k && j === l) return true;
  }
  return false;
}

// Checks that the given turn is valid for the given player, and returns
// a copy of the original pieces updated to reflect the turn, or null if the
// turn was invalid.
//
// Assumes turn has been parsed by parseTurn() so it's superfically valid.
export function validateTurn(oldPieces, player, turn) {
  const pieces = [...oldPieces];  // copy
  let i;
  for (i = 0; i < turn.length - 1; ++i) {
    const move = turn[i];
    if (!isValidMove(pieces, player, ...move)) return null;
    executeMoves(pieces, move);
  }
  const push = turn[i];
  if (!isValidPush(pieces, player, ...push)) return null;
  executeMoves(pieces, push);
  return pieces;
}

// Executes one or moves/pushs, each of which must be valid.
//
// Returns 0 or 1 to indicate the winner, or -1 if the game is not over.
export function executeMoves(pieces, ...moves) {
  for(const [src, dst] of moves) {
    // Allow no-op move.
    if(src === dst) continue;

    if (pieces[dst] === 0) {
      // Move.
      pieces[dst] = pieces[src];
      pieces[src] = 0;
    } else {
      // Remove anchor.
      for (let i = 0; i < pieces.length; ++i) {
        if (getPieceType(pieces[i]) === PieceType.ANCHOR) {
          pieces[i] = makePiece(getPlayerColor(pieces[i]), PieceType.PUSHER);
        }
      }

      // Push.
      let r = FIELD_ROW[dst];
      let c = FIELD_COL[dst];
      let dr = r - FIELD_ROW[src];
      let dc = c - FIELD_COL[src];
      let i = dst;
      let piece = makePiece(getPlayerColor(pieces[src]), PieceType.ANCHOR);
      pieces[src] = 0;
      do {
        let newPiece = pieces[i];
        pieces[i] = piece;
        piece = newPiece;
        r += dr;
        c += dc;
        i = getFieldIndex(r, c);
      } while (piece !== 0 && i !== -1);
      return piece === 0 ? -1 : getPlayerColor(piece);
    }
  }
  return -1;
}

// Like executeMoves(), but doesn't mutate the pieces argument and returns
// an updated copy instead.
export function applyMoves(pieces, ...moves) {
  const newPieces = [...pieces];  // copy
  executeMoves(newPieces, ...moves);
  return newPieces;
}

export function formatField(i) {
  return String.fromCharCode('a'.charCodeAt(0) + FIELD_COL[i]) + String(4 - FIELD_ROW[i]);
}

export function formatMove(src, dst) {
  return formatField(src) + '-' + formatField(dst);
}

export function formatTurn(parts) {
  return parts.map(([src, dst]) => formatMove(src, dst)).join(',');
}

// Parses `s` into a field index, or returns -1 if it's invalid.
export function parseField(s) {
  if (s.length !== 2) return -1;
  const c = s.charCodeAt(0) - 'a'.charCodeAt(0);
  const r = '4'.charCodeAt(0) - s.charCodeAt(1);
  return getFieldIndex(r, c);
}

export function parseMove(s) {
  const parts = s.split('-');
  if (parts.length !== 2) return undefined;
  const src = parseField(parts[0]);
  const dst = parseField(parts[1]);
  if (src < 0 || dst < 0) return undefined;
  return [src, dst];
}

export function parseTurn(s) {
  if (s.length === 0) return undefined;
  const parts = s.split(',');
  if (parts.length < 1 || parts.length > 3) return undefined;
  const moves = [];
  for (const part of parts) {
    const move = parseMove(part);
    if (move == null) return undefined;
    moves.push(move);
  }
  return moves;
}
