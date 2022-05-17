#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "perms.h"
#include "board.h"

#include <functional>
#include <vector>
#include <utility>

// Enumerates the successors of `perm`.
bool GenerateSuccessors(
    const Perm &perm,
    std::function<bool(const Moves&, const State&)> callback);

// Deduplicates successors that lead to the same state.
//
// Unused? TODO: delete this?
void Deduplicate(std::vector<std::pair<Moves, State>> &successors);

#endif  // ndef SEARCH_H_INCLUDED
