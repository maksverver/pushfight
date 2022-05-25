#include "accessors.h"
#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

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

// Accessor for the previous phase's results.
std::optional<RnAccessor> acc;

// Expected outcome for this phase.
//
// We detect all positions that are won-in-1 in phase 0.
//
// We detect all positions that are lost-in-2 in phase 1 (including positions
// lost because there are no moves available, which are technically lost-in-0).
//
// For phase N where N is even, we detect all wins-in-(N + 1).
// For phase N where N is odd, we detect all losses-in-(N + 1).
//
// So in a particular phase N, the outcome can only be WIN (if N is even)
// or loss (if N is odd).
Outcome expected_outcome = TIE;

Outcome Compute(const Perm &perm) {
  Outcome o = LOSS;
  bool complete = GenerateSuccessors(perm, [&o](const Moves&, const State& state) {
    // If there is an immediately winning/losing move, we should have skipped the computation.
    assert(state.outcome == TIE);

    Outcome p = (*acc)[IndexOf(state.perm)];
    o = MaxOutcome(o, p);
    return o != WIN;
  });
  assert(complete == (o != WIN));
  return o;
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
      Outcome o = (*acc)[perm_index];
      if (o == LOSS || o == WIN) {
        ++stats->kept;
      } else {
        o = Compute(perm);
        if (o == TIE) {
          ++stats->unchanged;
        } else {
          assert(o == expected_outcome);
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
  std::vector<uint8_t> bits(chunk_size / 8);
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

// Encodes outcomes into bytes, with 5 ternary values per byte (i.e., 8/5 = 1.6 bits per value).
std::vector<uint8_t> EncodeOutcomes(const std::vector<Outcome> &outcomes) {
  assert(outcomes.size() % 5 == 0);
  std::vector<uint8_t> bytes;
  bytes.reserve(outcomes.size() * 8 / 5);
  for (size_t i = 0; i < outcomes.size(); i += 5) {
    uint8_t byte = outcomes[i] +
        outcomes[i + 1]*3 +
        outcomes[i + 2]*3*3 +
        outcomes[i + 3]*3*3*3 +
        outcomes[i + 4]*3*3*3*3;
    bytes.push_back(byte);
  }
  return bytes;
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

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    std::cout << "Usage: solve-rN <phase> <start-chunk> <end-chunk>" << std::endl;
    return 0;
  }

  const int phase = ParseInt(argv[1]);
  if (phase < 2) {
    std::cout << "Invalid phase. Must be 2 or higher.\n";
    return 1;
  }

  const int start_chunk = argc > 2 ? std::max(0, ParseInt(argv[2])) : 0;
  const int end_chunk = argc > 3 ? std::min(ParseInt(argv[3]), num_chunks) : num_chunks;

  std::cout << "Calculating " << end_chunk - start_chunk << " R" << phase << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive) "
      << "using " << num_threads << " threads." << std::endl;

  acc.emplace(PhaseInputFilename(phase - 1).c_str());
  expected_outcome = phase % 2 == 0 ? WIN : LOSS;

  InitializePerms();
  FOR(chunk, start_chunk, end_chunk) {
    const std::string filename = ChunkFileName(phase, "output", chunk);
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
