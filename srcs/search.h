#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "board.h"
#include "perms.h"
#include "search-impl.h"

#include <functional>
#include <vector>
#include <utility>

// Enumerates the successors of `perm`.
//
// Precondition: `perm` must be a permutation that is started or in-progress.
//
// Callback is a callable of the form: bool(const Moves&, const State&).
//
// When the callback returns false, the search is aborted, and this function
// returns false too. If the callback returns true every time it is called,
// then all successors are enumerated and this function returns true.
template<class Callback>
bool GenerateSuccessors(const Perm &perm, Callback callback) {
  Perm mutable_perm = perm;
  Moves moves = {.size = 0, .moves={}};
  return
    impl::GenerateSuccessors(mutable_perm, moves, 0, callback) &&
    impl::GenerateSuccessors(mutable_perm, moves, 1, callback) &&
    impl::GenerateSuccessors(mutable_perm, moves, 2, callback);
}

// Enumerates the predecessors of `perm`.
//
// Note: this includes predecessors that are themselves unreachable!
void GeneratePredecessors(
    const Perm &perm,
    const std::function<void(const Perm&)> &callback);

// Enumerates the successors of `perm` and collects them in a vector.
std::vector<std::pair<Moves, State>> GenerateAllSuccessors(const Perm &perm);

// Deduplicates successors that lead to the same state.
void Deduplicate(std::vector<std::pair<Moves, State>> &successors);

// Returns whether there is an immediately-winning move in the given permutation.
//
// Modifies the argument during the computation, but restores it to its original
// value before returning.
bool HasWinningMove(Perm &perm);

#endif  // ndef SEARCH_H_INCLUDED
