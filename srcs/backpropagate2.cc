// Tool to backpropagate losses detected in phase N-1 to wins in phase N.
//
// The idea is to loop over all LOSSes, generate all predecessors, and mark them
// all as WINs.
//
// This may be faster than the forward search done by solve-rN, since there are
// significantly fewer LOSSes than remaining TIEs, while the number of
// predecessors of a position is similar to the number of successors.
//
// The downside is that this approach doesn't allow generating output in chunks,
// because predecessors may appear anywhere in the output.
//
// This implementation uses two preceding files r(N-2).bin and r(N-1).bin to
// calculate newly losing positions.

#include "auto-solver.h"
#include "accessors.h"
#include "board.h"
#include "bytes.h"
#include "chunks.h"
#include "dedupe.h"
#include "efcodec.h"
#include "flags.h"
#include "input-verification.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

const char *solver_id = "backpropagate2-v0.1.3";

// Number of threads to use for calculations. 0 to disable multithreading.
int num_threads = std::thread::hardware_concurrency();

std::optional<RnAccessor> rn2_acc;  // r(N-2).bin
std::optional<RnAccessor> rn1_acc;  // r(N-1).bin
int initialized_phase = -1;

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

const std::string ChunkOutputFilename(int phase, int chunk) {
  std::ostringstream oss;
  oss << "output/chunk-r" << phase << "-" << std::setfill('0') << std::setw(4) << chunk << "-wins.bin";
  return oss.str();
}

void InitPhase(int phase) {
  rn2_acc.emplace(PhaseInputFilename(phase - 2).c_str());
  rn1_acc.emplace(PhaseInputFilename(phase - 1).c_str());

  int failures =
      VerifyInputChunks(phase - 2, rn2_acc.value()) +
      VerifyInputChunks(phase - 1, rn1_acc.value());
  if (failures != 0) exit(1);

  initialized_phase = phase;
}

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

void ProcessPerm(int64_t perm_index, const Perm &perm, ChunkStats *stats, std::vector<int64_t> *wins) {
  {
    // Process new losses only.
    Outcome o1 = (*rn1_acc)[perm_index];
    if (o1 != LOSS) return;
    Outcome o2 = (*rn2_acc)[perm_index];
    if (o2 == LOSS) return;
    assert(o2 == TIE);
  }
  ++stats->losses_found;
  GeneratePredecessors(perm, [stats, wins](const Perm &pred){
    ++stats->total_predecessors;
    int64_t pred_index = IndexOf(pred);
    Outcome o = (*rn1_acc)[pred_index];
    if (o == TIE) {
      wins->push_back(pred_index);
      ++stats->wins_written;
    } else {
      assert(o == WIN);
    }
  });
}

void ComputeChunkThread(int chunk, std::atomic<int> *next_part, std::vector<int64_t> *wins, ChunkStats *stats) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    int64_t perm_index = start_index + part_start;
    Perm perm = PermAtIndex(perm_index);
    REP(i, part_size) {
      ProcessPerm(perm_index, perm, stats, wins);
      std::next_permutation(perm.begin(), perm.end());
      ++perm_index;
    }
  }
  SortAndDedupe(*wins);
}

ChunkStats ProcessChunk(int chunk, std::vector<int64_t> &wins) {
  std::atomic<int> next_part = 0;
  ChunkStats stats;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(chunk, &next_part, &wins, &stats);
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    std::vector<std::vector<int64_t>> thread_wins(num_threads);
    std::vector<ChunkStats> thread_stats(num_threads);
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, chunk, &next_part, &thread_wins[i],&thread_stats[i]);
    }
    REP(i, num_threads) {
      threads[i].join();
      wins.insert(wins.end(), thread_wins[i].begin(), thread_wins[i].end());
    }
    assert(next_part == num_parts + num_threads);
    for (const ChunkStats &s : thread_stats) stats.Merge(s);
  }
  ClearChunkUpdate();
  return stats;
}

bytes_t ComputeChunk(int phase, int chunk) {
  assert(phase == initialized_phase);

  auto start_time = std::chrono::system_clock::now();

  std::vector<int64_t> wins;
  ChunkStats stats = ProcessChunk(chunk, wins);
  SortAndDedupe(wins);
  bytes_t encoded_wins = EncodeEF(wins);

  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
  double elapsed_minutes = elapsed_seconds.count() / 60;
  std::cerr << "Chunk stats: "
    << stats.losses_found << " losses found. "
    << stats.wins_written << " wins written. "
    << wins.size() << " new wins.\n";

  if (stats.losses_found > 0) {
    std::cerr << "Average number of predecessors: " << stats.total_predecessors / stats.losses_found << ".\n";
  }
  std::cerr << "Chunk " << chunk << " done in " << elapsed_minutes << " minutes. "
      << "Solving speed: " << stats.losses_found / elapsed_minutes << " losses / minute." << std::endl;

  return encoded_wins;
}

void RunManually(int phase, int start_chunk, int end_chunk) {
  std::cout << "Calculating " << end_chunk - start_chunk << " R" << phase << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive) "
      << "using " << num_threads << " threads." << std::endl;
  FOR(chunk, start_chunk, end_chunk) {
    const std::string filename = ChunkOutputFilename(phase, chunk);
    if (std::filesystem::exists(filename)) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    WriteToFile(filename, ComputeChunk(phase, chunk));
  }
}

void PrintUsage() {
  std::cout << solver_id << "\n\n"
    << "For manual chunk assignment:\n\n"
    << "  backpropagate2 --phase=N --start=<start-chunk> --end=<end-chunk>\n\n"
    << "For automatic chunk assignment (requires network access):\n\n"
    << "  backpropagate2 --phase=N --user=<user-id> --machine=<machine-id>\n"
    << "      [--host=styx.verver.ch] [--port=7429]\n"
    << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  std::string arg_phase;
  std::string arg_start;
  std::string arg_end;
  std::string arg_host = "styx.verver.ch";
  std::string arg_port = "7429";
  std::string arg_user;
  std::string arg_machine;
  std::map<std::string, Flag> flags = {
    {"phase", Flag::required(arg_phase)},

    // For manual chunk assignment
    {"start", Flag::optional(arg_start)},
    {"end", Flag::optional(arg_end)},

    // For automatic chunk assignment
    {"host", Flag::optional(arg_host)},
    {"port", Flag::optional(arg_port)},
    {"user", Flag::optional(arg_user)},
    {"machine", Flag::optional(arg_machine)},
  };

  if (argc == 1) {
    PrintUsage();
    return 0;
  }

  if (!ParseFlags(argc, argv, flags)) {
    std::cout << "\n";
    PrintUsage();
    return 1;
  }

  if (argc > 1) {
    std::cout << "Too many arguments!\n\n";
    PrintUsage();
    return 1;
  }

  bool want_manual = !arg_start.empty() || !arg_end.empty();
  bool want_automatic = !arg_user.empty() || !arg_machine.empty();

  if ((!want_manual && !want_automatic) || (want_manual && want_automatic)) {
    std::cout << "Must provide either --start and --end flags, "
        << "or --user and --machine flags, but not both!\n\n";
    PrintUsage();
    return 1;
  }

  int phase = ParseInt(arg_phase.c_str());
  if (phase < 2) {
    std::cout << "Invalid phase. Must be 2 or higher.\n";
    return 1;
  }
  if (phase % 2 != 0) {
    std::cout << "Invalid phase. Must be an even number.\n";
    return 1;
  }

  if (want_manual) {
    if (arg_start.empty() || arg_end.empty()) {
      std::cout << "Must provide both start and end chunks.\n";
      return 1;
    }
    int start_chunk = std::max(ParseInt(arg_start.c_str()), 0);
    int end_chunk = std::min(ParseInt(arg_end.c_str()), num_chunks);

    InitPhase(phase);
    RunManually(phase, start_chunk, end_chunk);
  } else {
    assert(want_automatic);
    if (arg_user.empty() || arg_machine.empty()) {
      std::cout << "Must provide both user and machine flags.\n";
      return 1;
    }

    InitPhase(phase);
    AutomaticSolver solver(
        solver_id, arg_host, arg_port, arg_user, arg_machine,
        ChunkOutputFilename, ComputeChunk, phase);
    solver.Run();
  }
}
