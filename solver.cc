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

constexpr int chunk_size = 54054000;
constexpr int num_chunks = 7429;

static_assert(int64_t{chunk_size} * int64_t{num_chunks} == total_perms);

// Number of parts to split each chunk into. Each part is processed by 1 thread.
// A large number of parts ensures that all CPU cores are kept busy most of the
// time, but too many parts incur a lot of overhead.
constexpr int num_parts = 225;
constexpr int part_size = 240240;

static_assert(part_size * num_parts  == chunk_size);
static_assert(part_size % 16 == 0);

// Number of threads to use for calculations. 0 to disable multithreading.
constexpr int num_threads = 8;

void PrintChunkUpdate(int chunk, int part) {
  // Precompute output line to minimize formatting errors due to multiple
  // threads writing to std::cerr at the same time.
  char buf[100];
  snprintf(buf, sizeof(buf),
      "Chunk %d calculating... part %d / %d (%.2f%% done)\r",
      chunk, part, num_parts, 100.0*part / num_parts);
  std::cerr << buf << std::flush;
}

void ClearChunkUpdate() {
  std::cerr << "                                                            \r";
}

bool IsBlackInDanger(const Perm &p) {
  for (int i : DANGER_POSITIONS) {
    if (p[i] == BLACK_MOVER || p[i] == BLACK_PUSHER) return true;
  }
  return false;
}

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

void ComputeChunkThread(int chunk, std::atomic<int> *next_part, Outcome outcomes[]) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    Perm perm = PermAtIndex(start_index + part_start);
    REP(i, part_size) {
      outcomes[part_start + i] = CalculateR0(perm);
      std::next_permutation(perm.begin(), perm.end());
    }
  }
}

std::vector<Outcome> ComputeChunk(int chunk) {
  std::vector<uint8_t> bits(chunk_size / 8);
  std::vector<Outcome> outcomes(chunk_size, TIE);
  std::atomic<int> next_part = 0;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(chunk, &next_part, outcomes.data());
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::atomic<int> next_part = 0;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, chunk, &next_part, outcomes.data());
    }
    REP(i, num_threads) threads[i].join();
    assert(next_part == num_parts + num_threads);
  }
  ClearChunkUpdate();
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
  assert(os);
}

int64_t ParseInt(const char *s) {
  int64_t i = -1;
  std::istringstream iss(s);
  iss >> i;
  assert(iss);
  return i;
}

}  // namespace

int main(int argc, char *argv[]) {
  int start_chunk = argc > 1 ? ParseInt(argv[1]) : 0;
  int end_chunk = argc > 2 ? ParseInt(argv[2]) : num_chunks;

  InitializePerms();
  FOR(chunk, start_chunk, end_chunk) {
    if (std::filesystem::exists(FileName(chunk))) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    auto start_time = std::chrono::system_clock::now();
    ProcessChunk(chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Chunk " << chunk << " done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}
