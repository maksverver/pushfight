#include "board.h"

#include "macros.h"

#include <cstdlib>
#include <iostream>

namespace {

constexpr char FIELD_CHARS[] = {'.', 'o', 'O', 'x', 'X', 'Y'};

// For each field, lists pairs of neighbors in opposite directions
// (terminated by a pair of -1s).
//
// For example, field 1 has neighbors 0 (to the left), 2 (to the right) and
// 8 (below). The list below contains only the pair (0, 2) because the field
// below doesn't have a matching field above.
//
// This is used to implement IsReachable() efficiently.
//
// Generated with gen-neighbor-pairs.py
const signed char NEIGHBOR_PAIRS[26][6] = {
  { -1,  -1,  -1,  -1,  -1,  -1 },  //  0
  {  0,   2,  -1,  -1,  -1,  -1 },  //  1
  {  1,   3,  -1,  -1,  -1,  -1 },  //  2
  {  2,   4,  -1,  -1,  -1,  -1 },  //  3
  { -1,  -1,  -1,  -1,  -1,  -1 },  //  4
  { -1,  -1,  -1,  -1,  -1,  -1 },  //  5
  {  5,   7,  -1,  -1,  -1,  -1 },  //  6
  {  0,  15,   6,   8,  -1,  -1 },  //  7
  {  1,  16,   7,   9,  -1,  -1 },  //  8
  {  2,  17,   8,  10,  -1,  -1 },  //  9
  {  3,  18,   9,  11,  -1,  -1 },  // 10
  {  4,  19,  10,  12,  -1,  -1 },  // 11
  { -1,  -1,  -1,  -1,  -1,  -1 },  // 12
  { -1,  -1,  -1,  -1,  -1,  -1 },  // 13
  {  6,  21,  13,  15,  -1,  -1 },  // 14
  {  7,  22,  14,  16,  -1,  -1 },  // 15
  {  8,  23,  15,  17,  -1,  -1 },  // 16
  {  9,  24,  16,  18,  -1,  -1 },  // 17
  { 10,  25,  17,  19,  -1,  -1 },  // 18
  { 18,  20,  -1,  -1,  -1,  -1 },  // 19
  { -1,  -1,  -1,  -1,  -1,  -1 },  // 20
  { -1,  -1,  -1,  -1,  -1,  -1 },  // 21
  { 21,  23,  -1,  -1,  -1,  -1 },  // 22
  { 22,  24,  -1,  -1,  -1,  -1 },  // 23
  { 23,  25,  -1,  -1,  -1,  -1 },  // 24
  { -1,  -1,  -1,  -1,  -1,  -1 },  // 25
};

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
    if (true) {
      // Implementation using NEIGHBOR_PAIRS as the lookup table.
      for (const signed char *p = NEIGHBOR_PAIRS[i]; *p != -1; p += 2) {
        if ((perm[p[0]] == EMPTY) != (perm[p[1]] == EMPTY)) {
          return true;
        }
      }
    } else {
      // Implementation without using NEIGHBOR_PAIRS. Logically equivalent to the
      // above, but may be less efficient.
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
    }
    return false;
  }
  // No anchor found!
  abort(); // should be unreachable
  return false;
}

const char *OutcomeToString(Outcome o) {
  return o == WIN ? "WIN" : o == LOSS ? "LOSS" : o == TIE ? "TIE" : "INVALID OUTCOME";
}

std::ostream &operator<<(std::ostream &os, const PrettyPerm &pp) {
  const Perm &p = pp.perm;
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
      if (!pp.compact || ch != ' ') os << ch;
    }
    if (!pp.compact) {
      if (pp.coords) os << static_cast<char>('4' - r);
      os << '\n';
    }
  }
  if (!pp.compact && pp.coords) {
    REP(c, W) os << static_cast<char>('a' + c);
    os << '\n';
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const PrettyState &ps) {
  const State &s = ps.state;
  Outcome o = s.outcome;
  os << PrettyPerm{.perm=s.perm, .compact=ps.compact, .coords=ps.coords};
  const char *outcome = o == WIN ? "win" : o == LOSS ? "loss" : "indeterminate";
  if (ps.compact) {
    os << ' ' << outcome;
  } else {
    os << "Outcome: " << outcome << '\n';
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Perm &p) {
  return os << PrettyPerm{p};
}

std::ostream &operator<<(std::ostream &os, const State &s) {
  return os << PrettyState{s};
}

std::ostream &operator<<(std::ostream &os, const Moves &moves) {
  REP(i, moves.size) {
    if (i > 0) os << ',';
    os << FieldToId(moves.moves[i].first) << '-' << FieldToId(moves.moves[i].second);
  }
  return os;
}
