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

// For each field, lists its neighbors (terminated by -1).
// Generated with gen-neighbors.py
const signed char NEIGHBORS[26][5] = {
  {  1,   7,  -1,  -1,  -1 },  //  0
  {  0,   2,   8,  -1,  -1 },  //  1
  {  1,   3,   9,  -1,  -1 },  //  2
  {  2,   4,  10,  -1,  -1 },  //  3
  {  3,  11,  -1,  -1,  -1 },  //  4
  {  6,  13,  -1,  -1,  -1 },  //  5
  {  5,   7,  14,  -1,  -1 },  //  6
  {  0,   6,   8,  15,  -1 },  //  7
  {  1,   7,   9,  16,  -1 },  //  8
  {  2,   8,  10,  17,  -1 },  //  9
  {  3,   9,  11,  18,  -1 },  // 10
  {  4,  10,  12,  19,  -1 },  // 11
  { 11,  20,  -1,  -1,  -1 },  // 12
  {  5,  14,  -1,  -1,  -1 },  // 13
  {  6,  13,  15,  21,  -1 },  // 14
  {  7,  14,  16,  22,  -1 },  // 15
  {  8,  15,  17,  23,  -1 },  // 16
  {  9,  16,  18,  24,  -1 },  // 17
  { 10,  17,  19,  25,  -1 },  // 18
  { 11,  18,  20,  -1,  -1 },  // 19
  { 12,  19,  -1,  -1,  -1 },  // 20
  { 14,  22,  -1,  -1,  -1 },  // 21
  { 15,  21,  23,  -1,  -1 },  // 22
  { 16,  22,  24,  -1,  -1 },  // 23
  { 17,  23,  25,  -1,  -1 },  // 24
  { 18,  24,  -1,  -1,  -1 },  // 25
};

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
  WHITE_PUSHER,  // removes anchor!
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

// Returns whether the given permutation can possibly be reached through a
// sequence of valid moves from a valid starting position.
//
// A return value of `true` implies that the permutation is likely reachable,
// while `false` implies that the permutation is definitly unreachable.
bool IsReachable(const Perm &perm);

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

const char *OutcomeToString(Outcome o);

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
