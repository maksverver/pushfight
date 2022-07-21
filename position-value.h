#ifndef POSITION_VALUE_H_INCLUDED
#define POSITION_VALUE_H_INCLUDED

#include <iostream>

struct Value {
  static Value LossIn(int moves) { assert(moves >= 0); return Value(moves*2 + 1); }
  static Value WinIn(int moves) { assert(moves > 0); return Value(moves*2); }
  static Value Tie() { return Value(0); }

  Value() : byte(0) {}

  explicit Value(uint8_t byte) : byte(byte) {}

  // Returns +1 if the position is won, -1 if lost, or 0 if tied.
  int Sign() const {
    return byte == 0 ? 0 : (byte&1) ? -1 : +1;
  }

  // Returns the number of moves left (0 if tied, otherwise a nonnegative value)
  int Magnitude() const {
    return byte >> 1;
  }

  uint8_t byte;
};

inline Value operator-(const Value &v) {
  return Value(v.byte == 0 ? 0 : v.byte + 1);
}

inline bool operator==(const Value &a, const Value &b) {
  return a.byte == b.byte;
}

// Evaluates whether value `a` comes before value `b`. The sort order is
// descending by default (i.e., the best moves come first.)
//
// Winning is better than tying,which is better than losing.
//
// If both positions are winning, the one which takes fewer moves is better.
// If both positions are losing, the one which takes more moves is better.
inline bool operator<(const Value &a, const Value &b) {
  int x = a.Sign();
  int y = b.Sign();
  return x != y ? x > y : x > 0 ? a.Magnitude() < b.Magnitude() : x < 0 ? a.Magnitude() > b.Magnitude() : 0;
}

inline std::ostream &operator<<(std::ostream &os, const Value &v) {
  if (v.Sign() == 0) return os << "T";
  return os << "WL"[v.Sign() < 0] << v.Magnitude();
}

#endif // ndef POSITION_VALUE_H_INCLUDED
