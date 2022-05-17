#include "board.h"

#include "macros.h"

#include <iostream>

std::string FieldToId(int i) {
  std::string s(2, '\0');
  s[0] = 'a' + FIELD_COL[i];
  s[1] = '4' - FIELD_ROW[i];
  return s;
}

std::ostream &Dump(const Perm &p, std::ostream &os) {
  REP(r, H) {
    REP(c, W) {
      int i = BOARD_INDEX
    [r][c];
      os << (i >= 0 && i < p.size() ? (p[i] >= 0 && p[i] < chars.size() ? chars[p[i]] : '?') : ' ');
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
