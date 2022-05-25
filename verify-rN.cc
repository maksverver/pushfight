// Tool to verify the internal consistency of a ternary output file.
//
// For phase 1, this is a complement to verify-r1, which does an explicit
// minimax search for a random sample of permutations. This tool instead
// checks that the outcome recorded for a sampled permutation is consistent
// with the outcomes of its successors.
//
// Note that it only verifies WIN or LOSS results!
//
// This tool has two downsides. First, like all verification tools, it only
// verifies a random subset of permutation. Second, it only checks internal
// consistency, not correctness. For example, a completely empty file (all TIE)
// is internally consistent, but doesn't represent the correct outcome of a
// computation.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>

#include "accessors.h"
#include "board.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

namespace {

// Default number of permutations to test (can be overridden on the command line).
const int64_t default_num_probes = 1000000;

// This variable controls how many consecutive permutations to sample.
//
// Ideally, we would select (num_probes) distinct uniformly random permutation
// indices. However, this also makes the tool very I/O sensitive and therefore
// very slow in practice. So instead, we only pick (num_probes /
// num_consecutive_probes) random indices, and for each random index, we sample
// the next (num_consecutive_probes) indices. This way we can cover orders of
// magnitude more permutations than if we sampled completely randomly.
const int num_consecutive_probes = 10000;

std::random_device dev;
std::mt19937 rng(dev());

Outcome CalculateOutcome(const RnAccessor &acc, const Perm &perm) {
  Outcome o = LOSS;
  GenerateSuccessors(perm, [&o, &acc](const Moves &moves, const State &state) {
    (void) moves;  // unused

    Outcome p = state.outcome;
    if (p == TIE) p = acc[IndexOf(state.perm)];
    o = MaxOutcome(o, Invert(p));
    return o != WIN;
  });
  return o;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: verify-r1 <rN.bin> [<num-probes>]" << std::endl;
    exit(1);
  }
  const char *filename = argv[1];

  int64_t num_probes = argc > 2 ? ParseInt64(argv[2]) : default_num_probes;

  RnAccessor acc(filename);
  InitializePerms();
  int64_t index = 0;
  Perm perm = first_perm;
  int64_t count[3] = {0, 0, 0};
  std::uniform_int_distribution<int64_t> dist(0, total_perms - num_consecutive_probes);
  REP(i, num_probes) {
    if (i % num_consecutive_probes == 0) {
      index = dist(rng);
      perm = PermAtIndex(index);
    }
    const Outcome actual = acc[index];
    const Outcome expected = actual == TIE ? TIE : CalculateOutcome(acc, perm);
    ++count[expected];
    if (actual != expected) {
      std::cout << "Invalid outcome at index " << index << ": "
          << "expected " << OutcomeToString(expected) << "; "
          << "actual " << OutcomeToString(actual) << "!" << std::endl;
      std::cout << perm << std::endl;
      exit(1);
    }
    ++index;
    std::next_permutation(perm.begin(), perm.end());
  }
  std::cout << "\rFile " << filename << " verified with " << num_probes << " probes. "
      << count[WIN] << " win, " << count[TIE] << " tie, " << count[LOSS] << " loss."
      << std::endl;
}
