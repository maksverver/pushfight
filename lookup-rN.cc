// Tool to look up a value at a specific index in a rN output file,
// along with successor information.

#include <iostream>
#include <iomanip>

#include "accessors.h"
#include "board.h"
#include "macros.h"
#include "perms.h"
#include "parse-int.h"
#include "search.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: lookup-rN <rN.bin> <index>\n";
    return 0;
  }
  const char *filename = argv[1];
  int64_t index = ParseInt64(argv[2]);
  if (index < 0 || index >= total_perms) {
    std::cout << "Invalid permutation index: " << index << std::endl;
    return 1;
  }

  RnAccessor acc(filename);
  InitializePerms();
  Perm perm = PermAtIndex(index);

  std::cout << "Stored outcome: " << OutcomeToString(acc[index]) << std::endl;

  Outcome o = LOSS;
  std::vector<std::pair<Moves, State>> successors = GenerateAllSuccessors(perm);
  Deduplicate(successors);
  std::cout << "\n" << successors.size() << "Distinct successors:\n";
  Moves best_moves;
  best_moves.size = 0;
  for (const std::pair<Moves, State> &elem : successors) {
    const Moves &moves = elem.first;
    const State &state = elem.second;

    Outcome old_o = o;
    o = MaxOutcome(o, INVERSE_OUTCOME[state.outcome]);
    if (o != old_o) best_moves = moves;

    std::cout << IndexOf(state.perm) << ' ';
    Dump(moves, std::cout) << ' ' << OutcomeToString(state.outcome) << '\n';
  };

  std::cout << "\nComputed outcome: " << OutcomeToString(o) << "\nBest moves: ";
  Dump(best_moves, std::cout) << std::endl;
}
