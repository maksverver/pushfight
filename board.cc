#include "board.h"

#include "macros.h"

#include <cstdlib>
#include <iostream>

namespace {

constexpr char FIELD_CHARS[] = {'.', 'o', 'O', 'x', 'X', 'Y'};

std::string FieldToId(int i) {
  std::string s(2, '\0');
  s[0] = 'a' + FIELD_COL[i];
  s[1] = '4' - FIELD_ROW[i];
  return s;
}

}  // namespace

bool IsReachable(const Perm &perm) {
  // A permutation is impossible if the anchor is on a piece that couldn't have
  // pushed last turn. At a minimum, there must be a piece to one side and an
  // empty space on the opposide side.
  //
  // For example:
  //
  //    ..Yo.
  //
  // is valid (the previous state would be .Ox..)
  //
  //    .....
  //    .oYo.
  //    .....
  //
  // is invalid: the pusher cannot have pushed up or down, because there is no
  // piece there, and it cannot have pushed left or right, because it should
  // have left an empty space behind.
  REP(i, L) if (perm[i] == BLACK_ANCHOR) {
    const int r = FIELD_ROW[i];
    const int c = FIELD_COL[i];

    // Check vertical pushes.
    if (r > 0 && r < H - 1) {
      int j = BOARD_INDEX[r - 1][c];
      int k = BOARD_INDEX[r + 1][c];
      if (j >= 0 && k >= 0 && (perm[j] == EMPTY) != (perm[k] == EMPTY)) return true;
    }

    // Check horizontal pushes.
    if (c > 0 && c < W - 1) {
      int j = BOARD_INDEX[r][c - 1];
      int k = BOARD_INDEX[r][c + 1];
      if (j >= 0 && k >= 0 && (perm[j] == EMPTY) != (perm[k] == EMPTY)) return true;
    }

    return false;
  }
  // No anchor found!
  abort(); // should be unreachable
  return false;
}

std::ostream &Dump(const Perm &p, std::ostream &os) {
  REP(r, H) {
    REP(c, W) {
      int i = BOARD_INDEX[r][c];
      char ch;
      if (i >= 0 && i < p.size()) {
        int pi = p[i];
        ch = pi >= 0 && pi < std::size(FIELD_CHARS) ? FIELD_CHARS[pi] : '?';
      } else {
        ch = ' ';
      }
      os << ch;

    }
    os << '\n';
  }
  return os;
}

std::ostream &Dump(const State &s, std::ostream &os) {
  Outcome o = s.outcome;
  return Dump(s.perm, os) <<
      "Outcome: " << (o == WIN ? "win" : o == LOSS ? "loss" : "indeterminate") << '\n';
}

std::ostream &Dump(const Moves &moves, std::ostream &os) {
  REP(i, moves.size) {
    if (i > 0) os << ',';
    os << FieldToId(moves.moves[i].first) << '-' << FieldToId(moves.moves[i].second);
  }
  return os;
}
