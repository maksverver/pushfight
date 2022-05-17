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
  if (i < 0 || perm[i] == EMPTY) {
    // Must push at least one piece.
    return false;
  }
  while (i != -1 && perm[i] != EMPTY) {
    if (perm[i] == BLACK_ANCHOR) {
      // Cannot push anchored piece.
      return false;
    }
    r += dr;
    c += dc;
    if (r < 0 || r >= H) {
      // Canot push pieces past the anchor at the top/bottom of the board.
      return false;
    }
    i = getBoardIndex(r, c);
  }
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
    std::function<bool(const Moves&, const State&)> &callback) {
  if (moves_left > 0) {
    // Generate moves.
    std::vector<int> todo;
    REP(i0, L) if (perm[i0] == WHITE_MOVER || perm[i0] == WHITE_PUSHER) {
      // Optimization: don't move the same piece twice. There is never any reason for it.
      if (moves.size > 0 && moves.moves[moves.size - 1].second == i0) continue;

      todo.assign(1, i0);
      uint32_t visited = uint32_t{1} << i0;
      for (int j = 0; j < todo.size(); ++j) {
        const int i1 = todo[j];
        REP(d, 4) {
          if (const int i2 = getNeighbourIndex(i1, d); i2 >= 0 && perm[i2] == EMPTY && (visited & (uint32_t{1} << i2)) == 0) {
            visited |= uint32_t{1} << i2;
            todo.push_back(i2);

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

}  // namespace

bool GenerateSuccessors(
    const Perm &perm,
    std::function<bool(const Moves&, const State&)> callback) {
  Perm mutable_perm = perm;
  Moves moves = {.size = 0, .moves={}};
  return
    GenerateSuccessors(mutable_perm, moves, 0, callback) &&
    GenerateSuccessors(mutable_perm, moves, 1, callback) &&
    GenerateSuccessors(mutable_perm, moves, 2, callback);
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
