#include "board.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace {

int64_t ParseInt64(const char *s) {
  std::istringstream iss(s);
  int64_t index = -1;
  iss >> index;
  assert(iss);
  return index;
}

void DumpPerm(const Perm &perm) {
  Dump(perm, std::cout) << std::endl;

  std::cout << "Successors:\n\n";

  Outcome o = LOSS;
  GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
    Dump(moves, std::cout) << std::endl;
    Dump(state, std::cout) << std::endl;
    o = MaxOutcome(o, INVERSE_OUTCOME[state.outcome]);
    return true;
  });

  std::cout << "Predecessors:\n\n";
  GeneratePredecessors(perm, [](const Perm &perm) {
    Dump(perm, std::cout) << "\n";
  });

  std::cout << "Verdict: " << (o == WIN ? "win" : o == LOSS ? "loss" : "tie") << std::endl;
}

int count(const std::string &s, char target) {
  int n = 0;
  for (char ch : s) if (ch == target) ++n;
  return n;
}

}  // namespace

int main(int argc, char *argv[]) {
  InitializePerms();

  const std::string argv1 = argc > 1 ? argv[1] : "";

  if (argc == 3 && argv1 == "by-index") {
    int64_t index = ParseInt64(argv[2]);
    Perm perm = PermAtIndex(index);
      DumpPerm(perm);
  } else if (argc == 3 && argv1 == "by-perm") {
    std::string argv2 = argv[2];
    if (argv2.size() != 26) {
      std::cout << "Invalid length! Expected 26." << std::endl;
    } else if (count(argv2, 'o') != 2 || count(argv2, 'O') != 3 || count(argv2, 'x') != 2
        || count(argv2, 'X') != 2 || count(argv2, 'Y') != 1) {
      std::cout << "Invalid character counts." << std::endl;
    } else {
      Perm perm;
      REP(i, 26) {
        char ch = argv2[i];
        perm[i] = ch == 'o' ? WHITE_MOVER : ch == 'O' ? WHITE_PUSHER : ch == 'x' ? BLACK_MOVER :
            ch == 'X' ? BLACK_PUSHER : ch == 'Y' ? BLACK_ANCHOR : EMPTY;
      }
      std::cout << "Permutation index: " << IndexOf(perm) << std::endl;
      DumpPerm(perm);
    }
  } else {
    std::cout << "Usage:\n" <<
        "  print by-index 123\n"
        "  print by-perm .OX.....oxY....Oox.....OX.\n";
  }
}
