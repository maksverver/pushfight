// Defines methods for converting between Push Fight permutations and integers.
//
// A Push Fight board consists of 26 fields. It can be encoded as a string where:
//
//  0 is an empty space
//  1 is a white mover (2 total)
//  2 is a white pusher (3 total)
//  3 is a black mover (2 total)
//  4 is a black mover without an anchor (2 total)
//  5 is a black mover with an anchor (1 total)
//
// This assumes that black was the last player and white is the next player.
// Since the board and all moves are symmetric, positions where white was the
// last player (and black is the next player) can be obtained by simply swapping
// the colors of the pieces.
//
// Note that these strings always have the same characters, just in a different
// order. There are a total number of 401,567,166,000 different permutations.
// This file defines methods to convert from permutations to indices and back.

#ifndef PERMS_H_INCLUDED
#define PERMS_H_INCLUDED

#include <array>
#include <cstdint>

// Permutation length (L).
constexpr int L = 26;

// Total number of permutations of first_perm.
//
// 26! / 16! / 2! / 3! / 2! / 2! = 401,567,166,000
constexpr int64_t total_perms = 401567166000;

// A permutation of 26 values character
using Perm = std::array<char, L>;

// First permutation (of IN_PROGRESS permutations).
constexpr Perm first_perm = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5};

// Last permutation (of IN_PROGRESS permutations).
constexpr Perm last_perm = {5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

enum class PermType {
  // The permutation does not represent a valid game configuration.
  INVALID = 0,

  // All pieces have been placed on the board, but no move has been made yet.
  // Note: this does not verify that each player's pieces are placed on the
  // correct side of the board.
  STARTED = 1,

  // All pieces are on the board, and The anchor is placed on one piece.
  IN_PROGRESS = 2,

  // One piece has been pushed off the board.
  FINISHED = 3,
};

// Checks if the permutation is valid or not, and whether the position
// corresponds to a starting position, an in-progress position, or a finished
// position (see PermType above).
PermType ValidatePerm(const Perm &perm);

inline bool IsInvalid(const Perm &perm)    { return ValidatePerm(perm) == PermType::INVALID; }
inline bool IsStarted(const Perm &perm)    { return ValidatePerm(perm) == PermType::STARTED; }
inline bool IsInProgress(const Perm &perm) { return ValidatePerm(perm) == PermType::IN_PROGRESS; }
inline bool IsFinished(const Perm &perm)   { return ValidatePerm(perm) == PermType::FINISHED; }

// Returns the index of a given permutation. The permutation must represent an
// in-progress position (i.e. it must be some permutation of first_perm).
int64_t IndexOf(const Perm &p);

// Returns the permutation at a given index. The index must be valid (i.e., it
// must be between 0 and total_perms, exclusive).
Perm PermAtIndex(int64_t idx);

// Returns a copy of the permutation with pieces rotated by 180 degrees.
//
// This is equivalent to reversing the elements in the array.
Perm Rotated(const Perm &perm);

// Rotates the pieces by 180 degrees.
//
// This is equivalent to reversing the elements in the array.
void Rotate(Perm &perm);

// Number of minimized indices. Each minimized index between 0 and this value
// (exclusive), corresponds to exactly one reachable permutation, up to
// rotation.
constexpr int64_t min_index_size = 86208131520;

// Returns the minimized index for the given permutation.
//
// Minimized indices eliminate unreachable and rotated positions. Calculcating
// minimized indices is slightly more expensive than calculating regular
// indices with IndexOf() (though hopefully not prohibitively so).
//
// If `rotated` is not null, *rotated is updated to reflect whether the board
// had to be rotated to calculate the minimized index. This value can be passed
// to PermAtMindex() to restore the original permutation.
//
// The given permutation must be reachable (according to IsReachable() defined
// in board.h) or the result of this function is undefined.
int64_t MinIndexOf(const Perm &p, bool *rotated = nullptr);

// Returns the permutation at a given minimized index. The index must be valid
// (i.e., it must be between 0 and min_index_size, exclusive). This is the
// inverse of MinIndexOf().
//
// This function is not optimized for speed. It's intended mainly for testing
// and debugging.
Perm PermAtMinIndex(int64_t idx, bool rotated = false);

#endif  // ndef PERMS_H_INCLUDED
