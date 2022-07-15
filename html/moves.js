'use strict';

function* generateMoveDestinations(fields, initialField) {
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
      if (r2 >= 0 && r2 < H && c2 >= 0 && c2 < W) {
        const i2 = FIELD_INDEX[r2][c2];
        if (i2 >= 0 && getPieceType(fields[i2]) === PieceType.NONE) {
          if (!visited.has(i2)) {
            visited.add(i2);
            todo.push(i2);
            yield i2;
          }
        }
      }
    }
  }
}

function isValidPush(fields, i, d) {
  let r = FIELD_ROW[i];
  let c = FIELD_COL[i];
  const dr = DR[d];
  const dc = DC[d];
  r += dr;
  c += dc;
  i = getFieldIndex(r, c);
  if (i === -1 || fields[i] === 0) {
    // Must push at least one piece.
    return false;
  }
  while (i !== -1 && fields[i] !== 0) {
    if (getPieceType(fields[i]) === PieceType.ANCHOR) {
      // Cannot push anchored piece.
      return false;
    }
    r += dr;
    c += dc;
    if (r < 0 || r >= H) {
      // Canot push pieces past the anchor at the top/bottom of the board.
      return false;
    }
    i = getFieldIndex(r, c);
  }
  return true;
}

function* findPushDirections(fields, i) {
  for (let d = 0; d < 4; ++d) {
    if (isValidPush(fields, i, d)) {
      yield d;
    }
  }
}

function* findPushDestinations(fields, i) {
  const r = FIELD_ROW[i];
  const c = FIELD_COL[i];
  for (const d of findPushDirections(fields, i)) {
    yield getFieldIndex(r + DR[d], c + DC[d]);
  }
}

/*
function* generateMoves(fields, player) {
  for (let i = 0; i < fields.length; ++i) {
    if (getPieceType(fields[i]) !== PieceType.NONE && getPlayerColor(fields[i]) === player) {
      // Sort destinations to facilitate debugging.
      const js = Array.from(generateMoveDestinations(fields, i)).sort((a, b) => a - b);
      for (const j of js) {
        yield {m: [i, j]};
      }
    }
  }
}

function* generatePushes(fields, player) {
  for (let i = 0; i < fields.length; ++i) {
    if (getPieceType(fields[i]) === PieceType.PUSHER && getPlayerColor(fields[i]) === player) {
      for (const d of findPushDirections(fields, i)) {
        yield {p: [i, d]};
      }
    }
  }
}

function* generateAllMoves(state) {
  if (state.movesLeft > 0) {
    yield* generateMoves(state.fields, state.nextPlayer);
  }
  yield* generatePushes(state.fields, state.nextPlayer);
}
*/

// Executes one or moves/pushs, each of which must be valid.
//
// Returns 0 or 1 to indicate the winner, or -1 if the game is not over.
function executeMoves(pieces, ...moves) {
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
function applyMoves(pieces, ...moves) {
  const newPieces = [...pieces];  // copy
  executeMoves(newPieces, ...moves);
  return newPieces;
}

function formatField(i) {
  return String.fromCharCode('a'.charCodeAt(0) + FIELD_COL[i]) + String(4 - FIELD_ROW[i]);
}

function formatMove(src, dst) {
  return formatField(src) + '-' + formatField(dst);
}

function formatTurn(parts) {
  return parts.map(([src, dst]) => formatMove(src, dst)).join(',');
}

// Parses `s` into a field index, or returns -1 if it's invalid.
function parseField(s) {
  if (s.length !== 2) return -1;
  const c = s.charCodeAt(0) - 'a'.charCodeAt(0);
  const r = '4'.charCodeAt(0) - s.charCodeAt(1);
  return getFieldIndex(r, c);
}

function parseMove(s) {
  const parts = s.split('-');
  if (parts.length !== 2) return undefined;
  const src = parseField(parts[0]);
  const dst = parseField(parts[1]);
  if (src < 0 || dst < 0) return undefined;
  return [src, dst];
}

function parseTurn(s) {
  if (s.length === 0) return undefined;
  const parts = s.split(',');
  if (parts.length > 3) return undefined;
  return parts.map(parseMove);
}
