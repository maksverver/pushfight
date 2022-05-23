#include <cassert>
#include <iostream>
#include <sstream>

#include "board.h"
#include "perms.h"
#include "search.h"

namespace {

int64_t ParseInt64(const char *s) {
  std::istringstream iss(s);
  int64_t i = -1;
  iss >> i;
  assert(iss);
  return i;
}

Outcome Minimax(const Perm &perm, int max_depth) {
  if (max_depth == 0) return TIE;

  Outcome o = LOSS;
  GenerateSuccessors(perm, [&o, max_depth](const Moves &moves, const State &state) {
    (void) moves;  // unused
    Outcome p = state.outcome;
    if (state.outcome == TIE) {
      p = Minimax(state.perm, max_depth - 1);
    }
    o = MaxOutcome(o, INVERSE_OUTCOME[p]);
    return o != WIN;
  });
  return o;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: minimax <index>\n";
    return 0;
  }

  int64_t i = ParseInt64(argv[1]);

  InitializePerms();
  Perm perm = PermAtIndex(i);

  // Note: this prints states which are lost due to the player being unable to
  // move as LOSS in 1 (or 3, 5, etc.) instead of LOSS in 0 (or 2, 4, 6, etc.)
  // This shouldn't matter too much in practice.
  Outcome o = TIE;
  for (int depth = 1; depth <= 2; ++depth) {
    o = Minimax(perm, depth);
    if (o != TIE) {
      std::cout << (o == WIN ? "WIN" : o == LOSS ? "LOSS" : "???") << " in " << depth << std::endl;
      break;
    }
  }
  if (o == TIE) {
    std::cout << "No solution found. Possibly TIE?" << std::endl;
  }
}
