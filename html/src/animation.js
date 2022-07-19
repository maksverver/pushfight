import {
  DR, DC, FIELD_COUNT, getFieldIndex, FIELD_ROW, FIELD_COL,
  PieceType, getPieceType, getPlayerColor, makePiece, NO_PIECE,
} from './board.js';

// This executes moves, similar to applyMoves(), but it also calculates
// piece animations as a side effect.
//
// `moves` may be a single more or push, or one or more moves followed by
// a push, but it cannot include multiple turns (i.e., there can be at most
// one push, and it must happen at the end). Note that the moves must be
// valid in the position described by `oldPieces`!
//
// Returns a list [animations, pieces]
//
// `pieces` is a copy of `oldPieces` with the moves applied.
//
// `animations` is an object with several types of keys:
//
//    integers: animations for pieces at the given field positions (after the move!)
//    "anchor": animation for the anchor
//    "pushed": animation for the pushed piece.
//    "duration": total animation duration (in milliseconds)
//
// Each value is an object with "keyframes" and "options" as keys.
// The "pushed" object additionally has "piece" and "position" fields,
// indicating the piece type and the final position where it should be rendered.
export function generatePieceAnimations(oldPieces, ...moves) {
  const moveTime = 200; // milliseconds
  const pushTime = 800; // milliseconds
  const anchorMoveTime = 500; // milliseconds
  const vanishTime = 800; // milliseconds
  const pieces = [...oldPieces];

  // Uses breath-first search to find a path from `src` to `dst` that only covers
  // empty fields. Returns undefined if no such path exists. The result is an array
  // representing the path, starting with `src` and ending with `dst`.
  function findShortestPath(src, dst) {
    const visited = new Map();
    visited.set(src, src);
    const todo = [src];
    for (let i = 0; i < todo.length; ++i) {
      const i1 = todo[i];
      const r1 = FIELD_ROW[i1];
      const c1 = FIELD_COL[i1];
      for (let d = 0; d < 4; ++d) {
        const r2 = r1 + DR[d];
        const c2 = c1 + DC[d];
        const i2 = getFieldIndex(r2, c2);
        if (i2 >= 0 && getPieceType(pieces[i2]) === PieceType.NONE && !visited.has(i2)) {
          visited.set(i2, i1);
          if (i2 === dst) {
            // Solution found!
            const path = [];
            let i = dst;
            for (;;) {
              path.push(i);
              if (i === src) break;
              i = visited.get(i);
            }
            path.reverse();
            return path;
          }
          todo.push(i2);
        }
      }
    }
  }

  // For each piece, we keep a list of [time, row, col] pairs.
  const movements = oldPieces.map(p => getPieceType(p) === PieceType.NONE ? null : []);

  const animations = {};

  // Converts a list of  [time, position] pairs to an animation object with keyframes/options.
  // `m` must be nonempty! If `pushed` is true, the movements represent a piece that is pushed off
  // the board, and an additional vanishing animation is appended.
  function movementToAnimation(m, rEnd, cEnd, pushed) {
    const [tStart] = m[0];
    const [tEnd] = m[m.length - 1];
    const duration = tEnd - tStart + (pushed ? vanishTime : 0);
    const keyframes = [];
    if (pushed) {
      keyframes.push({offset: 0, visibility: 'visible'})
    }
    let lastLeft, lastTop;
    for (let [t, r, c] of m) {
      lastLeft = -100*(cEnd - c) + '%';
      lastTop = -100*(rEnd - r) + '%'
      keyframes.push({offset: (t - tStart) / duration, left: lastLeft, top: lastTop});
    }
    if (pushed) {
      // Make pushed piece disappear by scaling it down to zero.
      keyframes.push(
          {offset: (tEnd - tStart) / duration, transform: 'scale(1)'},
          {offset: 1, transform: 'scale(0)', left: lastLeft, top: lastTop});
    }
    const options = { delay: tStart, duration: duration, fill: 'backwards' };
    return { keyframes, options };
  }

  let oldAnchorIndex = -1;
  let newAnchorIndex = -1;
  let time = 0;
  for (const move of moves) {
    const [src, dst] = move;
    if (src === dst) {
      continue;
    }

    if (pieces[dst] === 0) {
      // Move.
      const path = findShortestPath(src, dst) || [src, dst];
      for (let i = 0; i + 1 < path.length; ++i) {
        const a = path[i], b = path[i + 1];
        pieces[b] = pieces[a];
        pieces[a] = 0;
        movements[b] = movements[a];
        movements[a] = null;
        movements[b].push(
            [time, FIELD_ROW[a], FIELD_COL[a]],
            [time += moveTime, FIELD_ROW[b], FIELD_COL[b]]);
      }
    } else {
      // Push.
      const dr = FIELD_ROW[dst] - FIELD_ROW[src];
      const dc = FIELD_COL[dst] - FIELD_COL[src];

      // Remove old anchor.
      for (let i = 0; i < pieces.length; ++i) {
        if (getPieceType(pieces[i]) === PieceType.ANCHOR) {
          oldAnchorIndex = i;
          pieces[i] = makePiece(getPlayerColor(pieces[i]), PieceType.PUSHER);
        }
      }

      let i = src;
      let r = FIELD_ROW[i];
      let c = FIELD_COL[i];
      let p = pieces[i];
      let m = movements[i];
      p = makePiece(getPlayerColor(p), PieceType.ANCHOR);  // place anchor
      pieces[i] = NO_PIECE;
      movements[i] = null;
      for (;;) {
        const r2 = r + dr;
        const c2 = c + dc;
        const j = getFieldIndex(r2, c2);
        m.push([time, r, c], [time + pushTime, r2, c2]);
        if (j === -1) {
          // This piece is pushed off the board!
          animations.pushed = movementToAnimation(m, r, c, true);
          animations.pushed.position = i;
          animations.pushed.piece = p;
          break;
        }
        const q = pieces[j];
        const n = movements[j];
        if (getPieceType(p) === PieceType.ANCHOR) {
          newAnchorIndex = j;
        }
        pieces[j] = p;
        movements[j] = m;
        if (getPieceType(q) === PieceType.NONE) {
          // End of push.
          break;
        }
        i = j;
        r = r2;
        c = c2;
        p = q;
        m = n;
      }
      time += pushTime;
    }
  }

  // Calculate animations from movements.
  for (let i = 0; i < FIELD_COUNT; ++i) {
    const m = movements[i];
    if (m == null || m.length === 0) continue;
    animations[i] = movementToAnimation(m, FIELD_ROW[i], FIELD_COL[i]);
  }

  // Anchor animation.
  if (newAnchorIndex >= 0) {
    let keyframes;
    if (oldAnchorIndex < 0) {
      // Fade in.
      keyframes = [{color: 'transparent'}, {color: 'white'}];
    } else {
      // Translate.
      const dr = FIELD_ROW[newAnchorIndex] - FIELD_ROW[oldAnchorIndex];
      const dc = FIELD_COL[newAnchorIndex] - FIELD_COL[oldAnchorIndex];
      keyframes = [{left: -100*dc + '%', top: -100*dr + '%'}, {left: 0, top: 0}];
    }
    const options = { delay: time, duration: anchorMoveTime, fill: 'backwards' };
    animations.anchor = { keyframes, options };
    time += anchorMoveTime;
  }

  animations.duration = time;

  return [animations, pieces];
}
