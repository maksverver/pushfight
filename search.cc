#include "search.h"

#include "macros.h"
#include "perms.h"
#include "board.h"

#include <cassert>
#include <functional>
#include <vector>
#include <utility>

namespace {

bool IsValidPush(const Perm &perm, int i, int d) {
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
Outcome ExecutePush(Perm &perm, int i, int d) {
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

bool GenerateSuccessors(
    Perm &perm,
    Moves &moves,
    int moves_left,
    const std::function<bool(const Moves&, const State&)> &callback) {
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

      if (!callback(moves, state)) return false;

      --moves.size;
    }
  }
  return true;
}

bool HasWinningMove(const int danger[], Perm &perm, int moves_left, int last_move) {
  // Check if any of black's pieces in danger can be pushed off the board.
  for (const int *p = danger; *p >= 0; ++p) REP(d, 4) {
    int r = FIELD_ROW[*p];
    int c = FIELD_COL[*p];
    const int dr = DR[d];
    const int dc = DC[d];
    if (r + dr >= 0 && r + dr < H && (c + dc < 0 || c + dc >= W || BOARD_INDEX[r + dr][c + dc] < 0)) {
      for (;;) {
        r -= dr;
        c -= dc;
        int i = getBoardIndex(r, c);
        if (i < 0 || perm[i] == BLACK_ANCHOR || perm[i] == EMPTY) break;
        if (perm[i] == WHITE_PUSHER) return true;
      }
    }
  }
  if (moves_left > 0) {
    // Generate moves.
    int todo_data[L];  // uninitialized for efficiency
    int todo_size;
    REP(i0, L) if (perm[i0] == WHITE_MOVER || perm[i0] == WHITE_PUSHER) {
      // Optimization: don't move the same piece twice. There is never any reason for it.
      if (last_move == i0) continue;

      todo_size = 0;
      todo_data[todo_size++] = i0;
      uint32_t visited = uint32_t{1} << i0;
      for (int j = 0; j < todo_size; ++j) {
        const int i1 = todo_data[j];
        REP(d, 4) {
          if (const int i2 = getNeighbourIndex(i1, d); i2 >= 0 && perm[i2] == EMPTY && (visited & (uint32_t{1} << i2)) == 0) {
            visited |= uint32_t{1} << i2;
            todo_data[todo_size++] = i2;

            std::swap(perm[i0], perm[i2]);
            bool res = HasWinningMove(danger, perm, moves_left - 1, i2);
            std::swap(perm[i0], perm[i2]);  // must restore `perm` before returning

            if (res) return true;
          }
        }
      }
    }
  }
  return false;
}

void GeneratePredecessors(
    Perm &perm,
    int moves_left,
    int last_dest,
    const std::function<void(const Perm&)> &callback) {
  if (moves_left > 0) {
    // Generate moves.
    int todo_data[L];  // uninitialized for efficiency
    int todo_size;
    REP(i0, L) if (perm[i0] == WHITE_MOVER || perm[i0] == WHITE_PUSHER) {
      // Optimization: don't move the same piece twice. There is never any reason for it.
      if (i0 == last_dest) continue;

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

            std::swap(perm[i0], perm[i2]);
            GeneratePredecessors(perm, moves_left - 1, i2, callback);
            std::swap(perm[i0], perm[i2]);
          }
        }
      }
    }
  } else {
    callback(perm);
  }
}

}  // namespace

bool GenerateSuccessors(
    const Perm &perm,
    const std::function<bool(const Moves&, const State&)> &callback) {
  Perm mutable_perm = perm;
  Moves moves = {.size = 0, .moves={}};
  return
    GenerateSuccessors(mutable_perm, moves, 0, callback) &&
    GenerateSuccessors(mutable_perm, moves, 1, callback) &&
    GenerateSuccessors(mutable_perm, moves, 2, callback);
}

void GeneratePredecessors(
    const Perm &input_perm,
    const std::function<void(const Perm&)> &callback) {
  REP(anchor_index, L) if (input_perm[anchor_index] == BLACK_ANCHOR) {
    REP(d, 4) {
      int i = anchor_index;
      int r = FIELD_ROW[i];
      int c = FIELD_COL[i];
      const int dr = DR[d];
      const int dc = DC[d];
      int j = getBoardIndex(r + dr, c + dc);
      if (j < 0 || input_perm[j] != EMPTY) continue;
      int k = getBoardIndex(r - dr, c - dc);
      if (k < 0 || input_perm[k] == EMPTY) continue;
      // Anchor at field `i` could have been pushed in direction `d`.

      // Flip position (replaces BLACK_ANCHOR with WHITE_PUSHER).
      Perm perm;
      REP(j, L) perm[j] = INVERSE_PIECE[int{input_perm[j]}];
      perm[j] = perm[i];
      j = i;
      i = k;
      r -= dr;
      c -= dc;
      uint32_t pushed = 0;
      do {
        pushed |= uint32_t{1} << j;
        perm[j] = perm[i];
        j = i;
        r -= dr;
        c -= dc;
        i = getBoardIndex(r, c);
        perm[j] = EMPTY;

        // The push didn't necessarily end at an empty space or the edge of the
        // board. For example, `.Yooo` has predecessors `Ox.xx`, `Oxx.x` and
        // `Oxxx.`, not just `Oxxx.` as it might first appear.
        REP(j, L) if (perm[j] == BLACK_PUSHER && ((pushed & (uint32_t{1} << j)) == 0)) {
          // Note: this includes some unreachable positions!
          perm[j] = BLACK_ANCHOR;

          GeneratePredecessors(perm, 0, -1, callback);
          GeneratePredecessors(perm, 1, -1, callback);
          GeneratePredecessors(perm, 2, -1, callback);

          perm[j] = BLACK_PUSHER;
        }
      } while (i >= 0 && perm[i] != EMPTY);
    }
  }
}

void Deduplicate(std::vector<std::pair<Moves, State>> &successors) {
  auto lt = [](const std::pair<Moves, State> &a, const std::pair<Moves, State> &b) {
    return a.second.perm < b.second.perm;
  };
  auto eq = [](const std::pair<Moves, State> &a, const std::pair<Moves, State> &b) {
    return a.second.perm == b.second.perm;
  };
  std::sort(successors.begin(), successors.end(), lt);
  std::vector<std::pair<Moves, State>>::iterator it = std::unique(successors.begin(), successors.end(), eq);
  successors.erase(it, successors.end());
}

bool HasWinningMove(Perm &perm) {
  // Check if any of black's pieces are in danger of being pushed off the board.
  int danger_data[std::size(DANGER_POSITIONS) + 1];  // uninitialized for efficiency
  int danger_size = 0;
  for (int i : DANGER_POSITIONS) {
    if (perm[i] == BLACK_MOVER || perm[i] == BLACK_PUSHER) {
      danger_data[danger_size++] = i;
    }
  }

  // If none of black's pieces are in danger, it's not possible to win immediately.
  if (danger_size == 0) return false;

  danger_data[danger_size] = -1;

  return HasWinningMove(danger_data, perm, 2, -1);
}
