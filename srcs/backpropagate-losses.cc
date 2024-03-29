// Tool to backpropagate losses detected in phase 1 to wins in phase 2.
//
// The idea is to loop over all LOSSes, generate all predecessors, and mark them
// all as WINs.
//
// This may be faster than the forward search done by solve-rN, since there are
// significantly fewer LOSSes than remaining TIEs, while the number of
// predecessors of a position is similar to the number of successors.
//
// For phase 2, this will use the losses in r1.bin directly. For phase 4 and up,
// it also needs a delta file from phase r(N-2).bin to phase r(N-1).bin, which
// marks the new losses discovered in the previous phase (N-1).
//
// The downside is that this approach doesn't allow generating output in chunks,
// because predecessors may appear anywhere in the output.

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
#include <unordered_set>
#include <thread>
#include <vector>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
int num_threads = std::thread::hardware_concurrency();

std::optional<RnAccessor> prev_input_acc;
std::optional<BinaryAccessor<total_perms / 8>> delta_input_acc;

// This accessor is thread-safe.
std::optional<MutableLossPropagationAccessor> output_acc;

struct ChunkStats {
  int64_t losses_found = 0;
  int64_t wins_written = 0;
  int64_t total_predecessors = 0;

  void Merge(const ChunkStats &s) {
    losses_found += s.losses_found;
    wins_written += s.wins_written;
    total_predecessors += s.total_predecessors;
  }
};

void ProcessPerm(int64_t perm_index, const Perm &perm, ChunkStats *stats) {
  if (delta_input_acc ?
      // phase 4, 6, 8 etc. require a delta file marking the new losses
      (*delta_input_acc)[perm_index] :
      // phase 2 can use the input from r1.bin directly.
      ((*prev_input_acc)[perm_index] == LOSS)) {
    ++stats->losses_found;
    GeneratePredecessors(
      perm,
      [stats](const Perm &pred){
        ++stats->total_predecessors;
        int64_t pred_index = IndexOf(pred);
        Outcome o = (*prev_input_acc)[pred_index];
        if (o == TIE) {
          output_acc->MarkWinning(pred_index);
          ++stats->wins_written;
        } else {
          assert(o == WIN);
        }
      });
  }
}

void ComputeChunkThread(int chunk, std::atomic<int> *next_part, ChunkStats *stats) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    int64_t perm_index = start_index + part_start;
    Perm perm = PermAtIndex(perm_index);
    REP(i, part_size) {
      ProcessPerm(perm_index, perm, stats);
      std::next_permutation(perm.begin(), perm.end());
      ++perm_index;
    }
  }
}

ChunkStats ProcessChunk(int chunk) {
  std::atomic<int> next_part = 0;
  ChunkStats stats;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(chunk, &next_part, &stats);
  } else {
    // Multi-threaded computation.
    std::atomic<int> next_part = 0;
    std::vector<std::thread> threads;
    std::vector<ChunkStats> thread_stats(num_threads);
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, chunk, &next_part, &thread_stats[i]);
    }
    REP(i, num_threads) threads[i].join();
    assert(next_part == num_parts + num_threads);
    for (const ChunkStats &s : thread_stats) stats.Merge(s);
  }
  ClearChunkUpdate();
  return stats;
}

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

const std::string DeltaInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << "-delta.bin";
  return oss.str();
}

const std::string OutputFilename(int phase) {
  std::ostringstream oss;
  oss << "output/r" << phase << "-wins.bin";
  return oss.str();
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    std::cout << "Usage: backpropagate-losses <phase> <start-chunk> <end-chunk>" << std::endl;
    return 0;
  }

  const int phase = ParseInt(argv[1]);
  if (phase < 2) {
    std::cout << "Invalid phase. Must be 2 or higher.\n";
    return 1;
  }
  if (phase % 2 != 0) {
    std::cout << "Invalid phase. Must be an even number.\n";
    return 1;
  }

  prev_input_acc.emplace(PhaseInputFilename(phase - 1).c_str());
  if (phase > 2) delta_input_acc.emplace(DeltaInputFilename(phase - 1).c_str());
  output_acc.emplace(OutputFilename(phase).c_str());

  const int start_chunk = argc > 2 ? std::max(0, ParseInt(argv[2])) : 0;
  const int end_chunk = argc > 3 ? std::min(ParseInt(argv[3]), num_chunks) : num_chunks;

  std::cout << "Backpropagating losses in " << end_chunk - start_chunk << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive)." << std::endl;

  FOR(chunk, start_chunk, end_chunk) {
    if (output_acc->IsChunkComplete(chunk)) {
      std::cerr << "Chunk " << chunk << " already done. Skipping..." << std::endl;
      continue;
    }
    auto start_time = std::chrono::system_clock::now();
    ChunkStats stats = ProcessChunk(chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    double elapsed_minutes = elapsed_seconds.count() / 60;
    std::cerr << "Chunk stats: "
      << stats.losses_found << " losses found. "
      << stats.wins_written << " wins written.\n";
    if (stats.losses_found > 0) {
      std::cerr << "Average number of predecessors: " << stats.total_predecessors / stats.losses_found << ".\n";
    }
    std::cerr << "Chunk " << chunk << " done in " << elapsed_minutes << " minutes. "
        << "Solving speed: " << stats.losses_found / elapsed_minutes << " losses / minute." << std::endl;
    output_acc->MarkChunkComplete(chunk);
  }
}
