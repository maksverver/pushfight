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
constexpr int64_t total_perms = 401567166000;

// A permutation of 26 characters, with 16 0s, 2 1s, 3 2s, 2 3s, 2 4s, and 1 5 (see all_freq below).
using Perm = std::array<char, L>;

// First permutation.
constexpr Perm first_perm = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5};

// Last permutation.
constexpr Perm last_perm = {5, 4, 4, 3, 3, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Frequencies of symbols in any permutation of first_perm.
constexpr std::array<int, 6> all_freq = {16, 2, 3, 2, 2, 1};


// Initializes lookup tables. This must be called before any of the other
// functions. Ideally, it's only called once, but it's possible to call it
// multiple times.
void InitializePerms();

// Returns the index of a given permutation. Note that the permutation must be
// valid (i.e., it must be some permutation of first_perm).
int64_t IndexOf(const Perm &p);

// Returns the permutation at a given index. Note that the index must be valud
// (i.e., it must be between 0 and total_perms, exclusive).
Perm PermAtIndex(int64_t idx);

#endif  // ndef PERMS_H_INCLUDED
