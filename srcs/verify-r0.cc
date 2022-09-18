#include "chunks.h"
#include "board.h"
#include "macros.h"
#include "random.h"
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

constexpr int num_probes = 1000;

std::mt19937 rng = InitializeRng();

void VerifyFile(const char *filename) {
  std::cout << "Verifying " << filename << "... ";
  int64_t filesize = std::filesystem::file_size(filename);
  std::vector<uint8_t> chunk_data(chunk_size / 8);
  if (filesize != chunk_data.size()) {
    std::cout << "Incorrect file size: " << filesize << " (expected: " << chunk_data.size() << ")" << std::endl;
    exit(1);
  }
  {
    std::ifstream ifs(filename, std::ifstream::binary);
    ifs.read(reinterpret_cast<char*>(chunk_data.data()), chunk_data.size());
    if (ifs.gcount() != chunk_data.size()) {
      std::cout << "Failed to read full file!" << std::endl;
      exit(1);
    }
  }

  ChunkInfo chunk_info = GetChunkInfo(filename);
  if (chunk_info.phase == -1 || chunk_info.chunk == -1) {
    std::cout << "Failed to parse chunk info" << std::endl;
    exit(1);
  }
  if (chunk_info.phase != 0) {
    std::cout << "Invalid phase" << std::endl;
    exit(1);
  }

  int64_t start_index = int64_t{chunk_size} * int64_t{chunk_info.chunk};
  REP(_, num_probes) {
    std::uniform_int_distribution<int> dist(0, chunk_size - 1);
    int offset = dist(rng);

    int64_t index = start_index + offset;
    Perm perm = PermAtIndex(index);

    Outcome actual = ((chunk_data[offset / 8] >> (offset % 8)) & 1) ? WIN : TIE;

    Outcome expected = LOSS;
    GenerateSuccessors(perm, [&expected](const Moves &moves, const State &state) {
      (void) moves;  // unused
      expected = MaxOutcome(expected, Invert(state.outcome));
      return true;
    });
    if (actual != expected) {
      std::cout << "Invalid outcome at offset " << offset << " (permutation index " << index << "): "
          << "expected " << OutcomeToString(expected) << "; "
          << "actual " << OutcomeToString(actual) << "!" << std::endl;
      std::cout << perm << std::endl;
      exit(1);
    }
  }
  std::cout << "\rFile " << filename << " verified with " << num_probes << " probes." << std::endl;
}

}  // namespace

// Tool to verify the integrity of an r0 chunk.
int main(int argc, char *argv[]) {
  if (argc < 1) {
    std::cout << "Usage: verify-r0 [file]..." << std::endl;
    exit(1);
  }
  FOR(i, 1, argc) VerifyFile(argv[i]);
}
