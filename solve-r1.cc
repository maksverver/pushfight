#include <cassert>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "accessors.h"
#include "board.h"
#include "codec.h"
#include "chunks.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
int num_threads = std::thread::hardware_concurrency();

struct ChunkStats {
  // Number of preexisting results (WIN/LOSS) that were kept.
  int64_t kept = 0;

  // Number of recomputed values (TIE) that were changed (to WIN/LOSS).
  int64_t changed = 0;

  // Number of recomputed values (TIE) that were unchanged (remained TIE).
  int64_t unchanged = 0;

  void Merge(const ChunkStats &s) {
    kept += s.kept;
    changed += s.changed;
    unchanged += s.unchanged;
  }
};

#ifdef CHUNKED_ACCESSOR
ChunkedR0Accessor r0acc;
#else
R0Accessor r0acc;
#endif

// For r0, the result can only be LOSS (if all successors are WIN for the opponent)
// or TIE, since the r0 chunks don't have losing information yet.
Outcome Compute(const Perm &perm) {
  bool complete = GenerateSuccessors(perm, [](const Moves&, const State& state) {
    // If there is an immediate winning move, we should have skipped the computation.
    assert(state.outcome != LOSS);

    // Skip states that are winning for the opponent (i.e. losing for me).
    if (state.outcome == WIN) return true;

    assert(state.outcome == TIE);

    if (r0acc[IndexOf(state.perm)]) {
      // Successor is won by opponent, which is a loss for me. Keep searching.
      return true;
    } else {
      // Successor is a tie. Abort the search because we cannot achieve better in phase 1.
      // For phase 2 and on we should continue to search for a LOSS for oppponent
      // (i.e., win for me).
      return false;
    }
  });
  return complete ? LOSS : TIE;
}

void ComputeChunkThread(int chunk, std::atomic<int> *next_part, Outcome outcomes[], ChunkStats *stats) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    int64_t perm_index = start_index + part_start;
    Perm perm = PermAtIndex(perm_index);
    REP(i, part_size) {
      Outcome o = r0acc[perm_index] ? WIN : TIE;
      if (o == WIN) {
        ++stats->kept;
      } else {
        o = Compute(perm);
        if (o == TIE) {
          ++stats->unchanged;
        } else {
          assert(o == LOSS);
          ++stats->changed;
        }
      }
      outcomes[part_start + i] = o;
      std::next_permutation(perm.begin(), perm.end());
      ++perm_index;
    }
  }
}

std::vector<Outcome> ComputeChunk(int chunk) {
  std::vector<Outcome> outcomes(chunk_size, TIE);
  std::atomic<int> next_part = 0;
  ChunkStats stats;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(chunk, &next_part, outcomes.data(), &stats);
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::atomic<int> next_part = 0;
    std::vector<std::thread> threads;
    std::vector<ChunkStats> thread_stats(num_threads);
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, chunk, &next_part, outcomes.data(), &thread_stats[i]);
    }
    REP(i, num_threads) threads[i].join();
    assert(next_part == num_parts + num_threads);
    for (const ChunkStats &s : thread_stats) stats.Merge(s);
  }
  ClearChunkUpdate();
  std::cerr << "Chunk stats: kept=" << stats.kept << " unchanged=" << stats.unchanged << " changed=" << stats.changed << std::endl;
  return outcomes;
}

void ProcessChunk(const std::string &filename, int chunk) {
  std::vector<Outcome> outcomes = ComputeChunk(chunk);
  std::vector<uint8_t> bytes = EncodeOutcomes(outcomes);
  std::ofstream os(filename, std::ofstream::binary);
  if (!os) {
    std::cerr << "Could not open output file: " << filename << std::endl;
    exit(1);
  }
  os.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
  assert(os);
}

}  // namespace

int main(int argc, char *argv[]) {
  int start_chunk = argc > 1 ? std::max(0, ParseInt(argv[1])) : 0;
  int end_chunk = argc > 2 ? std::min(ParseInt(argv[2]), num_chunks) : num_chunks;

  std::cout << "Calculating " << end_chunk - start_chunk << " R1 chunks from " << start_chunk << " to "
      << end_chunk << " (exclusive) using " << num_threads << " threads." << std::endl;

  InitializePerms();
  FOR(chunk, start_chunk, end_chunk) {
    std::string filename = ChunkFileName(1, "output", chunk);
    if (std::filesystem::exists(filename)) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    auto start_time = std::chrono::system_clock::now();
    ProcessChunk(filename, chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Chunk " << chunk << " done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}
