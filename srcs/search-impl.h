#include "board.h"
#include "macros.h"
#include "perms.h"

#include <cassert>
#include <functional>

namespace impl {

inline bool IsValidPush(const Perm &perm, int i, int d) {
  const int dr = DR[d];
  const int dc = DC[d];
  int r = FIELD_ROW[i] + dr;
  int c = FIELD_COL[i] + dc;
  i = getBoardIndex(r, c);
  int last_piece;
  if (i < 0 || (last_piece = perm[i]) == EMPTY) {
    // Must push at least one piece.
    return false;
  }
  while (last_piece != EMPTY) {
    if (last_piece == BLACK_ANCHOR) {
      // Cannot push anchored piece.
      return false;
    }
    r += dr;
    c += dc;
    if (r < 0 || r >= H) {
      // Cannot push pieces past the railing at the top/bottom of the board.
      return false;
    }
    i = getBoardIndex(r, c);
    if (i < 0) {
      // Don't allow moves that push a player's own piece off the board.
      return last_piece != WHITE_MOVER && last_piece != WHITE_PUSHER;
    }
    last_piece = perm[i];
  }
  // Push ends on an empty field.
  return true;
}

// Executes a push move, flipping the pieces so the black becomes white and vice versa, and
// returns whether the move returns in an instant win or loss for the opponent (i.e., if
// white pushes a black piece off the board, the result contains only 4 white pieces and the
// result is LOSS to indicate the next player loses.)
inline Outcome ExecutePush(Perm &perm, int i, int d) {
  // Flip position, replace anchor.
  REP(j, L) perm[j] = INVERSE_PIECE[int{perm[j]}];
  perm[i] = BLACK_ANCHOR;

  // Move pushed pieces.
  int r = FIELD_ROW[i];
  int c = FIELD_COL[i];
  int dr = DR[d];
  int dc = DC[d];
  int f = EMPTY;
  for (;;) {
    const int j = getBoardIndex(r, c);
    if (j == -1) {
      // Piece pushed over the edge of the board.
      assert(f != EMPTY);
      return f == WHITE_MOVER || f == WHITE_PUSHER ? LOSS : WIN;
    }
    int g = perm[j];
    perm[j] = f;
    f = g;
    if (f == EMPTY) {
      // No more pieces to move.
      return TIE;
    }
    r += dr;
    c += dc;
  }
}

// Possible optimization: deduplicate states after move(s), just before push?
// That would make deduplicating after the fact unnecessary.

template<class Callback>
bool GenerateSuccessors(Perm &perm, Moves &moves, int moves_left, Callback &callback) {
  if (moves_left > 0) {
    // Generate moves.
    int todo_data[L];  // uninitialized for efficiency
    int todo_size;
    REP(i0, L) if (perm[i0] == WHITE_MOVER || perm[i0] == WHITE_PUSHER) {
      // Optimization: don't move the same piece twice. There is never any reason for it.
      if (moves.size > 0 && moves.moves[moves.size - 1].second == i0) continue;

      todo_size = 0;
      todo_data[todo_size++] = i0;
      uint32_t visited = uint32_t{1} << i0;
      for (int j = 0; j < todo_size; ++j) {
        const int i1 = todo_data[j];
        for (const signed char *n = NEIGHBORS[i1]; *n != -1; ++n) {
          const int i2 = *n;
          if (perm[i2] == EMPTY && (visited & (uint32_t{1} << i2)) == 0) {
            visited |= uint32_t{1} << i2;
            todo_data[todo_size++] = i2;

            moves.moves[moves.size] = {i0, i2};
            ++moves.size;

            std::swap(perm[i0], perm[i2]);
            if (!GenerateSuccessors(perm, moves, moves_left - 1, callback)) return false;
            std::swap(perm[i0], perm[i2]);

            --moves.size;
          }
        }
      }
    }
  } else {
    // Generate push moves.
    REP(i, L) if (perm[i] == WHITE_PUSHER) REP(d, 4) if (IsValidPush(perm, i, d)) {
      moves.moves[moves.size] = {i, getNeighbourIndex(i, d)};
      ++moves.size;

      // Possible optimization: undo push instead of copying the permutation?
      State state = {.perm = perm, .outcome = TIE};
      state.outcome = ExecutePush(state.perm, i, d);

      if (!callback(const_cast<const Moves&>(moves), const_cast<const State&>(state))) return false;

      --moves.size;
    }
  }
  return true;
}

}  // namespace impl
