#include "board.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
constexpr int num_threads = 8;

bool IsBlackInDanger(const Perm &p) {
  for (int i : DANGER_POSITIONS) {
    if (p[i] == BLACK_MOVER || p[i] == BLACK_PUSHER) return true;
  }
  return false;
}

constexpr int chunk_size = 54054000;
constexpr int num_chunks = 7429;
static_assert(int64_t{chunk_size} * int64_t{num_chunks} == total_perms);

Outcome CalculateR0(const Perm &perm) {
  // Optimization: for the first pass, only check successors if there is a
  // pushable black piece on an edge spot. Otherwise, we cannot win this turn.
  // Note: this means we cannot detect losses due to being unable to move.
  // My hypothesis is that there are no such positions, but if there are, the
  // we should interpret TIE as "not a WIN (possibly a loss)".
  if (!IsBlackInDanger(perm)) return TIE;

  Outcome o = TIE;
  int n = 0;
  bool finished = GenerateSuccessors(perm, [&o, &n](const Moves &moves, const State &state) {
    (void) moves;  // unused
    ++n;
    if (state.outcome == LOSS) {
      o = WIN;
      return false;
    }
    return true;
  });
  assert(finished == (o == TIE));
  if (n == 0) {
    Dump(perm, std::cout);
  }
  assert(n > 0);  // hypothesis: there is always some valid move
  return o;
}

void ComputeChunkThread(int64_t offset, int size, Outcome *outcomes) {
  Perm perm = PermAtIndex(offset);
  REP(i, size) {
    *outcomes++ = CalculateR0(perm);
    std::next_permutation(perm.begin(), perm.end());
  }
}

std::vector<Outcome> ComputeChunk(int chunk) {
  std::vector<uint8_t> bits(chunk_size / 8);
  int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  std::vector<Outcome> outcomes(chunk_size, TIE);
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(start_index, chunk_size, outcomes.data());
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    const int thread_size = chunk_size / num_threads;
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, start_index + thread_size * i, thread_size,
          outcomes.data() + thread_size * i);
    }
    REP(i, num_threads) threads[i].join();
  }
  return outcomes;
}

std::string FileName(int chunk) {
  std::ostringstream oss;
  oss << "results/chunk-r0-" << std::setfill('0') << std::setw(4) << chunk << ".bin";
  return oss.str();
}

void ProcessChunk(int chunk) {
  std::vector<Outcome> outcomes = ComputeChunk(chunk);
  assert(outcomes.size() % 8 == 0);
  std::vector<uint8_t> bytes(outcomes.size() / 8, uint8_t{0});
  REP(i, outcomes.size()) {
    assert(outcomes[i] == TIE || outcomes[i] == WIN);
    if (outcomes[i] == WIN) bytes[i / 8] |= 1 << (i % 8);
  }
  std::string filename = FileName(chunk);
  std::ofstream os(filename, std::fstream::binary);
  if (!os) {
    std::cerr << "Could not open output file: " << filename << std::endl;
    exit(1);
  }
  os.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
}

}  // namespace

int main(int argc, char *argv[]) {
  int start_chunk = 0;
  if (argc == 2) {
    std::istringstream iss(argv[1]);
    iss >> start_chunk;
    assert(iss);
  }

  InitializePerms();
  FOR(chunk, start_chunk, num_chunks) {
    if (std::filesystem::exists(FileName(chunk))) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    std::cerr << "Chunk " << chunk << " calculating... " << std::flush;
    auto start_time = std::chrono::system_clock::now();
    ProcessChunk(chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}
