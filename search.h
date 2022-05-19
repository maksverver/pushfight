#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "perms.h"
#include "board.h"

#include <functional>
#include <vector>
#include <utility>

// Enumerates the successors of `perm`.
//
// When the callback returns false, the search is aborted, and this function
// returns false too. If the callback returns true every time it is called,
// then all successors are enumerated and this function returns true.
bool GenerateSuccessors(
    const Perm &perm,
    std::function<bool(const Moves&, const State&)> callback);

// Deduplicates successors that lead to the same state.
//
// Unused? TODO: delete this?
void Deduplicate(std::vector<std::pair<Moves, State>> &successors);

// Returns whether there is an immediately-winning move in the given permutation.
//
// Modifies the argument during the computation, but restores it to its original
// value before returning.
bool HasWinningMove(Perm &perm);

#endif  // ndef SEARCH_H_INCLUDED
