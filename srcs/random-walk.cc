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

template<class T>
const T &Choose(std::mt19937 &rng, const std::vector<T> &choices) {
  assert(!choices.empty());
  std::uniform_int_distribution<size_t> dist(0, choices.size() - 1);
  return choices.at(dist(rng));
}

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
    perm = Choose(rng, new_perms);
  }
}

// Randomly walks over nodes that are wins, without revisiting any previous
// positions. Prints out perm indices of positions visited.
void WalkWins(MinimizedAccessor &acc) {
  std::mt19937 rng = InitializeRng();
  Perm perm;
  for (;;) {
    std::uniform_int_distribution<int64_t> min_index_dist(0, min_index_size - 1);
    int64_t start_index = min_index_dist(rng);
    perm = PermAtIndex(start_index);
    Value value = LookupValue(acc, perm, nullptr).value();

    // Start from a win-in-5 or better.
    if (value.IsWin() && value.Magnitude() >= 5) break;
  }

  std::set<Perm> seen;
  for (;;) {
    // Print current value.
    {
      Value value = LookupValue(acc, perm, nullptr).value();
      assert(value.IsWin());
      std::cout << value << ' ' << IndexOf(perm) << std::endl;
    }

    // Pick a random winning successor (note: this might be a worse position,
    // e.g. we could go from win-in-3 to win-in-5).
    {
      std::vector<EvaluatedSuccessor> successors = LookupSuccessors(acc, perm, nullptr).value();
      assert(successors.front().value.IsWin());
      std::vector<Perm> new_perms;
      for (const EvaluatedSuccessor &s : successors) {
        if (!s.value.IsWin()) break;
        const Perm &perm = s.state.perm;
        if (!seen.count(perm)) new_perms.push_back(perm);
      }
      if (new_perms.empty()) break;
      perm = Choose(rng, new_perms);
    }

    if (IsFinished(perm)) break;

    // Print current value.
    {
      Value value = LookupValue(acc, perm, nullptr).value();
      assert(value.IsLoss());
      std::cout << value << ' ' << IndexOf(perm) << std::endl;
    }

    // Pick an optimal losing successor that we haven't seen before.
    {
      std::vector<EvaluatedSuccessor> successors = LookupSuccessors(acc, perm, nullptr).value();
      assert(successors.front().value.IsLoss());
      std::vector<Perm> new_perms;
      for (const EvaluatedSuccessor &s : successors) {
        if (!new_perms.empty() && s.value != successors.front().value) break;
        const Perm &perm = s.state.perm;
        if (!seen.count(perm)) new_perms.push_back(perm);
      }
      if (new_perms.empty()) break;
      perm = Choose(rng, new_perms);
    }
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
  WalkWins(acc);

  if (false) WalkTies(acc);  // prevent unused code warning
}
