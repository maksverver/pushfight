// Verifies the results of a phase 1 chunk (chunk-r1-????.bin).
//
// Uses an independent minimax search to calculate the output, so this should be
// pretty reliable as verification, but it only samples a small subset of the
// outcomes in the chunk, so it doesn't prove the chunk is entirely correct.
//
// Since permutations are sampled randomly, it's possible to run the verifier
// multiple times to increase confidence of correctness.

#include "accessors.h"
#include "chunks.h"
#include "board.h"
#include "macros.h"
#include "search.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace {

constexpr int num_probes = 100000;

std::random_device dev;
std::mt19937 rng(dev());

// Does a 2-ply minimax search to calculate the exact answer.
//
// This only works for phase up to 1 because after that the number of positions
// grows much too large.
Outcome Calculate(const Perm &perm) {
  // 1 ply search for immediate win, or loss (i.e., no moves).
  {
    Outcome o = LOSS;
    GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
      (void) moves;  // unused
      o = MaxOutcome(o, INVERSE_OUTCOME[state.outcome]);
      return o != WIN;
    });
    if (o != TIE) return o;
  }

  {
    // 2 ply search.
    Outcome o = LOSS;
    GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
      (void) moves;  // unused
      Outcome p = LOSS;
      GenerateSuccessors(state.perm, [&o, &p](const Moves &moves, const State &state) {
        (void) moves;  // unused
        p = MaxOutcome(p, INVERSE_OUTCOME[state.outcome]);
        return p != WIN && !(p == TIE && o == TIE);  // alpha-beta pruning
      });
      o = MaxOutcome(o, INVERSE_OUTCOME[p]);
      return o != WIN;
    });
    return o;
  }
}

void VerifyFile(const char *filename) {
  std::cout << "Verifying " << filename << "... ";
  ChunkInfo chunk_info = GetChunkInfo(filename);
  if (chunk_info.phase == -1 || chunk_info.chunk == -1) {
    std::cout << "Failed to parse chunk info" << std::endl;
    exit(1);
  }
  if (chunk_info.phase != 1) {
    std::cout << "Invalid phase" << std::endl;
    exit(1);
  }
  int64_t filesize = std::filesystem::file_size(filename);
  if (filesize != chunk_size / 5) {
    std::cout << "Incorrect file size: " << filesize << " (expected: " << chunk_size / 5 << ")" << std::endl;
    exit(1);
  }
  RnChunkAccessor acc(filename);

  int count[3] = {0,0,0};
  int64_t start_index = int64_t{chunk_size} * int64_t{chunk_info.chunk};
  REP(_, num_probes) {
    std::uniform_int_distribution<int> dist(0, chunk_size - 1);
    int offset = dist(rng);
    int64_t index = start_index + offset;
    Perm perm = PermAtIndex(index);
    Outcome actual = acc[offset];
    Outcome expected = Calculate(perm);
    ++count[expected];
    if (actual != expected) {
      std::cout << "Invalid outcome at offset " << offset << " (permutation index " << index << "): "
          << "expected " << OutcomeToString(expected) << "; "
          << "actual " << OutcomeToString(actual) << "!" << std::endl;
      Dump(perm, std::cout) << std::endl;
      exit(1);
    }
  }
  std::cout << "\rFile " << filename << " verified with " << num_probes << " probes. "
      << count[WIN] << " win, " << count[TIE] << " tie, " << count[LOSS] << " loss."
      << std::endl;
}

}  // namespace

// Tool to verify the integrity of an r0 chunk.
int main(int argc, char *argv[]) {
  InitializePerms();
  if (argc < 1) {
    std::cout << "Usage: verify-r1 [file]..." << std::endl;
    exit(1);
  }
  FOR(i, 1, argc) VerifyFile(argv[i]);
}
