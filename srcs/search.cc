#include "search.h"

#include "macros.h"
#include "perms.h"
#include "board.h"

#include <cassert>
#include <functional>
#include <vector>
#include <utility>

namespace {

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

}  // namespace

std::vector<std::pair<Moves, State>> GenerateAllSuccessors(const Perm &perm) {
  std::vector<std::pair<Moves, State>> result;
  GenerateSuccessors(perm, [&result](const Moves &moves, const State &state) {
    result.emplace_back(moves, state);
    return true;
  });
  return result;
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
      // The push didn't necessarily end at an empty space or the edge of the
      // board. For example, `.Yooo` has predecessors `Ox.xx`, `Oxx.x` and
      // `Oxxx.`, not just `Oxxx.` as it might first appear.
      do {
        pushed |= uint32_t{1} << j;
        perm[j] = perm[i];
        j = i;
        r -= dr;
        c -= dc;
        i = getBoardIndex(r, c);
        perm[j] = EMPTY;

        // Select the previously anchored piece, which could be any of the
        // black pushers that haven't just been pushed.
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
    return a.second.perm < b.second.perm ||
      // In case of a tie, prefer to keep the shortest move sequence.
      (a.second.perm == b.second.perm && a.first.size < b.first.size);
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

bool PartialHasWinningMove(const Perm &perm) {
  bool seen[L] = {};
  int queue[L];  // uninitialized for efficiency
  int queue_size = 0;

  // Fill the queue with empty spaces where, if white moved a pusher there, it
  // could push a black piece off the board.
  for (int i : DANGER_POSITIONS) {
    if (perm[i] == BLACK_MOVER || perm[i] == BLACK_PUSHER) {
       REP(d, 4) {
        int r = FIELD_ROW[i];
        int c = FIELD_COL[i];
        const int dr = DR[d];
        const int dc = DC[d];
        if (r + dr >= 0 && r + dr < H && (c + dc < 0 || c + dc >= W || BOARD_INDEX[r + dr][c + dc] < 0)) {
          for (;;) {
            r -= dr;
            c -= dc;
            int i = getBoardIndex(r, c);
            if (i < 0 || perm[i] == BLACK_ANCHOR) break;  // not pushable
            if (perm[i] == EMPTY) {
              // Empty space found where white might be able to move a pusher.
              if (!seen[i]) {
                seen[i] = true;
                queue[queue_size++] = i;
              }
              break;
            }
            if (perm[i] == WHITE_PUSHER) return true;  // definitely pushable!
          }
        }
      }
    }
  }

  // Breadth-first search for a white pusher that can move to a free space that
  // allows pushing a black piece off the board.
  for (int queue_pos = 0; queue_pos < queue_size; ++queue_pos) {
    int i = queue[queue_pos];
    assert(perm[i] == EMPTY);
    REP(d, 4) {
      int j = getNeighbourIndex(i, d);
      if (j < 0) continue;
      if (!seen[j]) {
        if (perm[j] == WHITE_PUSHER) return true;
        if (perm[j] == EMPTY) {
          seen[j] = true;
          queue[queue_size++] = j;
        }
      }
    }
  }

  // We could extend the above logic to compute HasWinningMove() completely, by
  // marking reachable white movers too, and then checking if they can be moved
  // aside to clear a path for a white pusher to get next to the unanchored
  // black piece, but the logic is a bit complicated. Note that we'd also have
  // to consider the case where the white mover is moved next to the piece (or
  // sequence of pieces) and then the white pusher pushes both the white mover,
  // some other pieces, and finally pushes the black piece off the board.
  // Very complicated!
  //
  // So instead of doing all that, we'll just return pessimistically `false`
  // here.
  return false;
}
