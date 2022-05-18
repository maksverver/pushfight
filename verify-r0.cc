#include "board.h"
#include "macros.h"
#include "search.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace {

constexpr int chunk_size = 54054000;
constexpr int num_chunks = 7429;

constexpr int num_probes = 1000;

std::random_device dev;
std::mt19937 rng(dev());

const char *OutcomeToString(Outcome o) {
  return o == WIN ? "WIN" : o == LOSS ? "LOSS" : "TIE";
}

int GetChunkId(const char *filename) {
  for (const char *p = filename; *p; ++p) if (*p == '/') filename = p + 1;
  if (strncmp(filename, "chunk-r0-", 9) == 0) {
    std::istringstream iss(filename + 9);
    int i = 0;
    if (iss >> i) return i;
  }
  std::cout << "Couldn't parse chunk id from filename: " << filename << std::endl;
  exit(1);
  return -1;
}

void VerifyFile(const char *filename) {
  int64_t filesize = std::filesystem::file_size(filename);
  std::vector<uint8_t> chunk_data(chunk_size / 8);
  if (filesize != chunk_data.size()) {
    std::cout << "Incorrect file size: " << filesize << " (expected: " << chunk_data.size() << ")" << std::endl;
    exit(1);
  }
  {
    std::ifstream ifs(filename, std::ios::binary);
    ifs.read(reinterpret_cast<char*>(chunk_data.data()), chunk_data.size());
    if (ifs.gcount() != chunk_data.size()) {
      std::cout << "Failed to chunk file!" << std::endl;
      exit(1);
    }
  }

  const int chunk = GetChunkId(filename);
  if (chunk < 0 || chunk >= num_chunks) {
    std::cout << "Invalid chunk id " << chunk << std::endl;
    exit(1);
  }

  int64_t start_index = int64_t{chunk_size} * int64_t{chunk};
  REP(_, num_probes) {
    std::uniform_int_distribution<int> dist(0, chunk_size - 1);
    int offset = dist(rng);

    int64_t index = start_index + offset;
    Perm perm = PermAtIndex(index);

    Outcome actual = ((chunk_data[offset / 8] >> (offset % 8)) & 1) ? WIN : TIE;

    Outcome expected = LOSS;
    GenerateSuccessors(perm, [&expected](const Moves &moves, const State &state) {
      (void) moves;  // unused
      expected = MaxOutcome(expected, INVERSE_OUTCOME[state.outcome]);
      return true;
    });
    if (actual != expected) {
      std::cout << "Invalid outcome at offset " << offset << " (permutation index " << index << "): "
          << "expected " << OutcomeToString(expected) << "; "
          << "actual " << OutcomeToString(actual) << "!" << std::endl;
      Dump(perm, std::cout) << std::endl;
      exit(1);
    }
  }
  std::cout << "File " << filename << " verified with " << num_probes << " probes." << std::endl;
}

}  // namespace

// Tool to verify the integrity of an r0 chunk.
int main(int argc, char *argv[]) {
  InitializePerms();
  if (argc < 1) {
    std::cout << "Usage: verify-r0 [file]..." << std::endl;
    exit(1);
  }
  FOR(i, 1, argc) VerifyFile(argv[i]);
}
