#ifndef BOARD_H_INCLUDED
#define BOARD_H_INCLUDED

#include "perms.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

constexpr int H = 4;
constexpr int W = 8;

constexpr std::array<std::array<int, W>, H> BOARD_INDEX = {
  std::array<int, 8>{ -1, -1,  0,  1,  2,  3,  4, -1 },
  std::array<int, 8>{  5,  6,  7,  8,  9, 10, 11, 12 },
  std::array<int, 8>{ 13, 14, 15, 16, 17, 18, 19, 20 },
  std::array<int, 8>{ -1, 21, 22, 23, 24, 25, -1, -1 },
};

constexpr int DANGER_POSITIONS[] = {
  0, 4, 5, 6, 12, 13, 19, 20, 21, 25
};

constexpr std::array<int, L> FIELD_ROW = {
          0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,
  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3, 3,
};

constexpr std::array<int, L> FIELD_COL = {
          2,  3,  4,  5,  6,
  0,  1,  2,  3,  4,  5,  6,  7,
  0,  1,  2,  3,  4,  5,  6,  7,
      1,  2,  3,  4,  5
};

const int DR[4] = { -1,  0,  0, 1 };
const int DC[4] = {  0, -1, +1, 0 };

constexpr int EMPTY        = 0;
constexpr int WHITE_MOVER  = 1;
constexpr int WHITE_PUSHER = 2;
constexpr int BLACK_MOVER  = 3;
constexpr int BLACK_PUSHER = 4;
constexpr int BLACK_ANCHOR = 5;

constexpr const char INVERSE_PIECE[6] = {
  EMPTY,
  BLACK_MOVER,
  BLACK_PUSHER,
  WHITE_MOVER,
  WHITE_PUSHER,
  WHITE_PUSHER,
};

constexpr Perm initial_state = {
        0, 2, 4, 0, 0,
  0, 0, 0, 1, 3, 5, 0, 0,
  0, 0, 2, 1, 3, 0, 0, 0,
     0, 0, 2, 4, 0,
};


inline int getBoardIndex(int r, int c) {
  return r >= 0 && r < H && c >= 0 && c < W ? BOARD_INDEX[r][c] : -1;
}

inline int getNeighbourIndex(int i, int d) {
  return getBoardIndex(FIELD_ROW[i] + DR[d], FIELD_COL[i] + DC[d]);
}

enum Outcome {
  TIE  = 0,
  LOSS = 1,
  WIN  = 2,
};

inline Outcome MaxOutcome(Outcome a, Outcome b) {
  if (a == WIN || b == WIN) return WIN;
  if (a == TIE || b == TIE) return TIE;
  return LOSS;
}

constexpr const Outcome INVERSE_OUTCOME[3] = { TIE, WIN, LOSS };

struct State {
  Perm perm;
  Outcome outcome;
};

struct Moves {
  // Number of moves followed by a final push. Between 1 and 3 (inclusive).
  int size;

  // The actual moves. Each pair is a field index: from, to. Where `from` is
  // the index of a field occupied by the current player, and `to` is a field
  // that is empty (for move moves) or an adjacent occupied field
  // (for push moves).
  std::array<std::pair<int, int>, 3> moves;
};

std::ostream &Dump(const Perm &p, std::ostream &os);
std::ostream &Dump(const State &s, std::ostream &os);
std::ostream &Dump(const Moves &moves, std::ostream &os);

#endif  // ndef BOARD_H_INCLUDED
