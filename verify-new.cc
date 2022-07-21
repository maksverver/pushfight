// Tool to verify the internal consistency of a ternary output file.
//
// This is similar to verify-rN but it uses two .bin files to only verify
// values that were changed.
//
// It verifies two things:
//
//  1. It checks that all changes were TIE->WIN or TIE->LOSS (no regressions).
//     This is similar to encode-delta except this tool allows both new wins
//     and losses at the same time (which happens with solve2 output).
//  2. For a random sample of transitions, it recalculates the value and
//     checks that the stored value is consistent with the successors.
//
// Usage:
//
//  ./verify-new <old.bin> <new.bin> [<sample-ratio>]
//
// Where sample ratio is specified as the denominator x in 1/x  (e.g., "10"
// will verify 1 in 10 positions). With the default sample ratio of 1/100 the
// tool takes about 3 hours to run. With a sample ratio of 1/1000 it's about
// half of that.
//
// (old.bin may be a pipe instead of a file, since we only need to access
// it linearly.)
//
// This tool has two downsides.
//
//  1. Like all verification tools, it only verifies a random subset of
//     the input file. However, it can be run multiple times (or with lower
//     sampling to increase confidence that the input file is correct.)
//  2. It only checks internal consistency, not correctness. For example,
//     a completely empty file (all TIE) s internally consistent, but doesn't
//     represent the correct outcome of a computation.
//

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <random>

#include "accessors.h"
#include "board.h"
#include "codec.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

namespace {

constexpr int default_sample_ratio = 100;

std::random_device dev;
std::mt19937 rng(dev());

std::ifstream OpenInputUnbuffered(const char *filename) {
  std::ifstream ifs;
  ifs.rdbuf()->pubsetbuf(nullptr, 0);
  ifs.open(filename, std::ifstream::binary);
  return ifs;
}

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
  if (argc < 3 || argc > 4) {
    std::cout << "Usage: verify-new <r(N-1).bin> <rN.bin> [<sample_ratio>]" << std::endl;
    return 1;
  }
  const char *old_filename = argv[1];
  const char *new_filename = argv[2];
  int sample_ratio = argc > 3 ? ParseInt(argv[3]) : default_sample_ratio;
  if (sample_ratio < 1) {
    std::cerr << "Invalid sample ratio" << std::endl;
    return 1;
  }

  std::ifstream old_ifs = OpenInputUnbuffered(old_filename);
  if (!old_ifs) {
    std::cerr << "Failed to open " << old_filename << " for reading." << std::endl;
    return 1;
  }
  TernaryReader old_reader(old_ifs);

  RnAccessor acc(new_filename);

  // Evaluate 1/sample_ratio changed positions (at random).
  std::uniform_int_distribution<int64_t> dist(0, sample_ratio);

  int64_t index = 0;
  Perm perm = first_perm;
  int64_t changes[3] = {0, 0, 0};
  int64_t checked = 0;
  while (old_reader.HasNext()) {
    Outcome o1 = old_reader.Next();
    Outcome o2 = acc[index];
    if (o1 != o2) {
      if (o1 != TIE) {
        std::cerr << "Invalid transition at index " << index << ": "
            << OutcomeToString(o1) << " -> " << OutcomeToString(o2) << "!" << std::endl;
        return 1;
      }
      assert(o2 != TIE);
      ++changes[o2];
      if (dist(rng) == 0) {
        ++checked;
        Outcome o3 = CalculateOutcome(acc, perm);
        if (o2 != o3) {
          std::cerr << "Invalid outcome at index " << index << ": stored "
              << OutcomeToString(o2) << ", calculated " << OutcomeToString(o3) << "!" << std::endl;
          return 1;
        }
      }
    }
    ++index;
    std::next_permutation(perm.begin(), perm.end());
    if (index % chunk_size == 0) {
      std::cerr << "Chunk " << index / chunk_size << " / " << num_chunks << " done..." << std::endl;
    }
  }
  std::cout << "\rFile " << new_filename << " verified with " << checked << " probes. "
      << changes[WIN] << " new wins, " << changes[LOSS] << " new losses, out of "
      << index << " permutations." << std::endl;
}
