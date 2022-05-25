#include "search.h"

#include "macros.h"
#include "perms.h"

#ifdef NDEBUG
#error "Can't compile test with -DNDEBUG!"
#endif
#include <assert.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>

namespace {

std::random_device dev;
std::mt19937 rng(dev());

}  // namespace

int main() {
  InitializePerms();
  const int num_cases = 25;
  int64_t num_successors = 0;
  int64_t num_predecessors = 0;
  REP(n, num_cases) {
    std::uniform_int_distribution<int64_t> dist(0, total_perms - 1);
    int64_t index = dist(rng);
    Perm perm = PermAtIndex(index);

    GenerateSuccessors(perm,
      [index, &perm, &num_successors](const Moves &moves, const State &state) {
        (void) moves;  // unused
        ++num_successors;

        if (state.outcome != TIE) {
          // If a piece is pushed off the board, we cannot generate predecessors.
          return true;
        }

        const Perm &successor = state.perm;
        bool found = false;

        GeneratePredecessors(successor, [&found, &perm](const Perm &predecessor) {
          if (predecessor == perm) found = true;
        });
        if (!found) {
          std::cerr << "Failed to find predecessor of a successor!\n\n";
          std::cerr << "Original permutation (index " << index << "):\n";
          std::cerr << perm << '\n';
          std::cerr << "Successor (index " << IndexOf(successor) << "):\n";
          std::cerr << successor << '\n';
          exit(1);
        }
        return true;
      });

    GeneratePredecessors(perm,
      [index, &perm, &num_predecessors](const Perm &predecessor) {
        ++num_predecessors;
        bool complete = GenerateSuccessors(predecessor,
          [&perm](const Moves &moves, const State &state) {
            (void) moves;  // unused
            return state.perm != perm;
          });
          if (complete) {
            std::cerr << "Failed to find successor of a predecessor!\n\n";
            std::cerr << "Original permutation (index " << index << "):\n";
            std::cerr << perm << '\n';
            std::cerr << "Predecessor (index " << IndexOf(predecessor) << "):\n";
            std::cerr << predecessor << '\n';
            exit(1);
          }
          return true;
        });
  }
  std::cerr << "Tested " << num_cases << " random cases." << std::endl;
  std::cerr << "Average number of succcessors: " << num_successors / num_cases << std::endl;
  std::cerr << "Average number of predecessors: " << num_predecessors / num_cases << std::endl;
}
