// Tool to explore random walks across the game graph without revisiting
// positions.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <string>

#include "board.h"
#include "minimized-accessor.h"
#include "minimized-lookup.h"
#include "perms.h"
#include "position-value.h"
#include "random.h"

namespace {

// Randomly walks over nodes that are ties, without revisiting any previous
// positions. Prints out perm indices of positions visited.
//
// This generates a practically unlimited sequence.
void WalkTies(MinimizedAccessor &acc) {
  std::mt19937 rng = InitializeRng();
  Perm perm;
  do {
    std::uniform_int_distribution<int64_t> min_index_dist(0, min_index_size - 1);
    int64_t start_index = min_index_dist(rng);
    perm = PermAtIndex(start_index);
  } while (!LookupValue(acc, perm, nullptr).value().IsTie());

  std::set<Perm> seen;
  for (;;) {
    seen.insert(perm);
    std::cout << IndexOf(perm) << std::endl;

    std::vector<EvaluatedSuccessor> successors = LookupSuccessors(acc, perm, nullptr).value();
    assert(successors.front().value.IsTie());
    std::vector<Perm> new_perms;
    for (const EvaluatedSuccessor &s : successors) {
      if (!s.value.IsTie()) break;
      const Perm &perm = s.state.perm;
      if (!seen.count(perm)) new_perms.push_back(perm);
    }
    if (new_perms.empty()) break;
    std::uniform_int_distribution<size_t> dist(0, new_perms.size() - 1);
    perm = new_perms.at(dist(rng));
  }
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr <<
        "Usage: lookup-min <minimized.bin>\n";
    return 1;
  }

  MinimizedAccessor acc(argv[1]);
  WalkTies(acc);
}
