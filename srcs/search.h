#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "board.h"
#include "perms.h"
#include "search-impl.h"

#include <functional>
#include <vector>
#include <utility>

// An upper bound on the maximum number of successors for any position.
//
// Calculated as (1 + 5 * 16 + 5 * 4 / 2  * 16 * 16) * (3 * 3),
// where the first factor is the maximum total number of moves (assuming 0,
// 1 or 2 of the total 5 pieces are being moved to any of the 16 free squares)
// and the second term is the total number of pushes (any of the 3 pushers times
// 3 directions). Pushers can push in 3 rather than 4 directions because due to
// the shape of the board, for any position, a pusher can only push up or down,
// but never both.
//
// In practice, positions have around 4000 distinct successors on average (around
// double that without deduplication) with outliers over 8,000 (e.g. index
// 1707612174 has 18,431 possible turns, and 8412 distinct successors).
constexpr int max_successors = 23769;

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
    (moves.size = 1, impl::GenerateSuccessors(mutable_perm, moves, 0, callback)) &&
    (moves.size = 2, impl::GenerateSuccessors(mutable_perm, moves, 0, callback)) &&
    (moves.size = 3, impl::GenerateSuccessors(mutable_perm, moves, 0, callback));
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

// Partial version of HasWinningMove().
//
// If this function returns `true`, then there is an immediate winning move
// and HasWinningMove(perm) would return `true` as well.
//
// If this function returns `false` instead, there may or may not be an
// immediate winning move. Thus, this function can be used to quickly identify
// win-in-1s, which make up about 84.9% of all permutations, but it cannot be
// used to show that a position is not won-in-1.
//
// The current version identifies approximately 96.6% of randomly-selected
// permutations that are win-in-1.
bool PartialHasWinningMove(const Perm &perm);

#endif  // ndef SEARCH_H_INCLUDED
