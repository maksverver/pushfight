#include "board.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <iostream>
#include <sstream>

int64_t ParseInt(const char *s) {
  std::istringstream iss(s);
  int64_t index = -1;
  iss >> index;
  assert(iss);
  return index;
}

int main(int argc, char *argv[]) {
  InitializePerms();

  if (argc == 3 && std::string(argv[1]) == "by-index") {
    int64_t index = ParseInt(argv[2]);
    Perm perm = PermAtIndex(index);
    Dump(perm, std::cout) << std::endl;

    Outcome o = LOSS;
    GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
      Dump(moves, std::cout) << std::endl;
      Dump(state, std::cout) << std::endl;
      o = MaxOutcome(o, INVERSE_OUTCOME[state.outcome]);
      return true;
    });
    std::cout << "Verdict: " << (o == WIN ? "win" : o == LOSS ? "loss" : "tie") << std::endl;
  } else {
    std::cout << "Usage:\n" <<
        "  print by-index 123\n";
  }
}
