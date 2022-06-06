// Solver for two phases at once.
//
// This combines the logic from solve-rN (used to detect losses in an odd phase)
// with the logic from backpropgate2 (used to detect wins in an even phase).
//
// Given an even phase number N, it uses the input r(N-2).bin to generate new
// losses (output can be used to generate r(N-1).bin) and then backprogates
// those losses to calculate new wins (can be used to generate rN.bin).
//
// This works because to generate new losses we need to scan all undermined
// positions in r(N-2).bin, and to backpropagate losses, we only need to know
// wins that already existed in r(N-2).bin, since r(N-1).bin only contains new
// losses, not wins.
//
// Input file required for --phase=N:
//
//   input/r(N-2).bin
//
// Chunk output file name:
//
//    chunk-r<N>-two.bin
//
// Ouput file format:
//
//    EF-coded stream of newly-losing permutations
//    EF-coded stream of newly-winning permutations
//

#include "auto-solver.h"
#include "accessors.h"
#include "board.h"
#include "bytes.h"
#include "chunks.h"
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

constexpr const char *solver_id = "solve2-v0.1.3";
constexpr const char *default_hostname = "styx.verver.ch";
constexpr const char *default_portname = "7429";

// Number of threads to use for calculations. 0 to disable multithreading.
int num_threads = std::thread::hardware_concurrency();

std::optional<RnAccessor> acc;  // r(N-1).bin

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

const std::string ChunkOutputFilename(int phase, int chunk) {
  std::ostringstream oss;
  oss << "output/chunk-r" << phase << "-" << std::setfill('0') << std::setw(4) << chunk << "-two.bin";
  return oss.str();
}

void InitPhase(int phase) {
  acc.emplace(PhaseInputFilename(phase - 2).c_str());
  int failures = VerifyInputChunks(phase - 2, acc.value());
  if (failures != 0) exit(1);
}

//
// Loss computation logic starts here. This is equivalent to solve-rN.cc
//

struct ChunkStats1 {
  // Number of preexisting results (WIN/LOSS) that were already determined.
  int64_t skipped = 0;

  // Number of recomputed values (TIE) that were changed to LOSS.
  int64_t changed = 0;

  // Number of recomputed values (TIE) that were unchanged (remained TIE).
  int64_t unchanged = 0;

  void Merge(const ChunkStats1 &s) {
    skipped += s.skipped;
    changed += s.changed;
    unchanged += s.unchanged;
  }
};

void ComputeLoss(
    int64_t perm_index, const Perm &perm,
    std::vector<int64_t> *losses, ChunkStats1 *stats) {
  Outcome o = (*acc)[perm_index];
  // Only check undetermined positions.
  if (o == LOSS || o == WIN) {
    ++stats->skipped;
    return;
  }

  // A permutation is losing if all successors are winning (for the opponent).
  // So we can abort the search as soon as we find one non-winning successor.
  bool complete = GenerateSuccessors(perm, [](const Moves&, const State& state) {
    // If there is an immediately winning/losing move, we should have skipped the computation.
    assert(state.outcome == TIE);
    Outcome p = (*acc)[IndexOf(state.perm)];
    assert(p != LOSS);
    return p == WIN;
  });
  if (!complete) {
    ++stats->unchanged;
    return;
  }
  ++stats->changed;
  losses->push_back(perm_index);
}

void ComputeLossesThread(
    int chunk, std::atomic<int> *next_part,
    std::vector<int64_t> *losses,
    ChunkStats1 *stats) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    int64_t perm_index = start_index + part_start;
    Perm perm = PermAtIndex(perm_index);
    REP(i, part_size) {
      ComputeLoss(perm_index, perm, losses, stats);
      std::next_permutation(perm.begin(), perm.end());
      ++perm_index;
    }
  }
}

ChunkStats1 ComputeLosses(int chunk, std::vector<int64_t> &losses) {
  std::atomic<int> next_part = 0;
  ChunkStats1 stats;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeLossesThread(chunk, &next_part, &losses, &stats);
  } else {
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    std::vector<std::vector<int64_t>> thread_losses(num_threads);
    std::vector<ChunkStats1> thread_stats(num_threads);
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(ComputeLossesThread, chunk, &next_part, &thread_losses[i], &thread_stats[i]);
    }
    REP(i, num_threads) {
      threads[i].join();
      losses.insert(losses.end(), thread_losses[i].begin(), thread_losses[i].end());
      stats.Merge(thread_stats[i]);
    }
    assert(next_part == num_parts + num_threads);
  }
  ClearChunkUpdate();
  std::sort(losses.begin(), losses.end());
  assert(std::unique(losses.begin(), losses.end()) == losses.end());
  return stats;
}

// Win computation logic starts here. This is equivalent backpropgate2.cc

struct ChunkStats2 {
  int64_t total_predecessors = 0;

  void Merge(const ChunkStats2 &s) {
    total_predecessors += s.total_predecessors;
  }
};

void BackpropagateLoss(
    int64_t perm_index, const Perm &perm,
    std::vector<int64_t> *wins, ChunkStats2 *stats) {
  assert((*acc)[perm_index] == TIE);
  GeneratePredecessors(perm, [stats, wins](const Perm &pred){
    ++stats->total_predecessors;
    int64_t pred_index = IndexOf(pred);
    Outcome o = (*acc)[pred_index];
    if (o == TIE) {
      wins->push_back(pred_index);
    } else {
      assert(o == WIN);
    }
  });
}

void SortAndDedupe(std::vector<int64_t> &v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

void ComputeWinsThread(
    int chunk, const std::vector<int64_t> *losses, std::atomic<size_t> *next_loss,
    std::vector<int64_t> *wins, ChunkStats2 *stats) {
  const size_t num_losses = losses->size();
  for (;;) {
    size_t i = (*next_loss)++;
    if (i + 1 >= num_threads) {
      int progress = i + 1 - num_threads;
      if (progress % 1000 == 0) PrintChunkUpdate(chunk, progress, num_losses);
    }
    if (i >= num_losses) break;  // note: will actually exceed losses.size()!
    int64_t perm_index = (*losses)[i];
    Perm perm = PermAtIndex(perm_index);
    BackpropagateLoss(perm_index, perm, wins, stats);
  }
  SortAndDedupe(*wins);
}

ChunkStats2 ComputeWins(
    int chunk, const std::vector<int64_t> &losses,
    std::vector<int64_t> &wins) {
  std::atomic<size_t> next_loss = 0;
  ChunkStats2 stats;
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeWinsThread(chunk, &losses, &next_loss, &wins, &stats);
    assert(next_loss == losses.size() + 1);
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    std::vector<std::vector<int64_t>> thread_wins(num_threads);
    std::vector<ChunkStats2> thread_stats(num_threads);
    threads.reserve(num_threads);
    REP(i, num_threads) {
      threads.emplace_back(
        ComputeWinsThread, chunk, &losses, &next_loss,
        &thread_wins[i], &thread_stats[i]);
    }
    REP(i, num_threads) {
      threads[i].join();
      wins.insert(wins.end(), thread_wins[i].begin(), thread_wins[i].end());
      stats.Merge(thread_stats[i]);
    }
    assert(next_loss == losses.size() + num_threads);
  }
  ClearChunkUpdate();
  SortAndDedupe(wins);
  return stats;
}

// Combined logic continues here.

bytes_t ComputeChunk(int chunk) {
  auto start_time = std::chrono::system_clock::now();

  std::vector<int64_t> losses;
  std::vector<int64_t> wins;
  ChunkStats1 stats1 = ComputeLosses(chunk, losses);
  std::cerr << "Loss computation stats: "
      << stats1.skipped << " skipped. "
      << stats1.unchanged << " unchanged. "
      << stats1.changed << " new losses. " << std::endl;

  if (losses.size() > 0) {
    ChunkStats2 stats2 = ComputeWins(chunk, losses, wins);

    std::cerr << "Win computation stats: "
        << wins.size() << " new wins. "
        << stats2.total_predecessors / losses.size() << " average predecessors.";
    std::cerr << '\n';
  }

  bytes_t result;
  EncodeEF(losses, result);
  EncodeEF(wins, result);

  std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
  double elapsed_minutes = elapsed_seconds.count() / 60;
  std::cerr << "Chunk " << chunk << " done in " << elapsed_minutes << " minutes. " << std::endl;

  return result;
}

void RunManually(int phase, int start_chunk, int end_chunk) {
  std::cout << "Calculating " << end_chunk - start_chunk
      << " R" << phase << "+R" << phase + 1 << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive) "
      << "using " << num_threads << " threads." << std::endl;
  FOR(chunk, start_chunk, end_chunk) {
    const std::string filename = ChunkOutputFilename(phase, chunk);
    if (std::filesystem::exists(filename)) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    WriteToFile(filename, ComputeChunk(chunk));
  }
}

void PrintUsage() {
  std::cout << solver_id << "\n\n"
    << "For manual chunk assignment:\n\n"
    << "  backpropagate2 --phase=N --start=<start-chunk> --end=<end-chunk>\n\n"
    << "For automatic chunk assignment (requires network access):\n\n"
    << "  backpropagate2 --phase=N --user=<user-id> --machine=<machine-id>\n"
    << "      [--host=" << default_hostname << "] [--port=" << default_portname << "]\n"
    << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  InitializePerms();

  std::string arg_phase;
  std::string arg_start;
  std::string arg_end;
  std::string arg_host = default_hostname;
  std::string arg_port = default_portname;
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
        solver_id, phase, arg_host, arg_port, arg_user, arg_machine,
        [phase](int chunk) {
          return ChunkOutputFilename(phase, chunk);
        },
        ComputeChunk);
    solver.Run();
  }
}
