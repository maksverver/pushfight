#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "auto-solver.h"
#include "accessors.h"
#include "board.h"
#include "bytes.h"
#include "codec.h"
#include "chunks.h"
#include "flags.h"
#include "input-verification.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

namespace {

const char *solver_id = "solve-rN-v0.1.1";

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
  if (expected_outcome == LOSS) {
    // A permutation is losing if all successors are winning (for the opponent).
    // So we can abort the search as soon as we find one non-winning successor.
    bool complete = GenerateSuccessors(perm, [](const Moves&, const State& state) {
      // If there is an immediately winning/losing move, we should have skipped the computation.
      assert(state.outcome == TIE);
      Outcome p = (*acc)[IndexOf(state.perm)];
      assert(p != LOSS);
      return p == WIN;
    });
    return complete ? LOSS : TIE;
  } else {
    // A permutation is winning if any successor is losing (for the opponent).
    // So we can abort the search as soon as we find a losing position.
    assert(expected_outcome == WIN);
    bool complete = GenerateSuccessors(perm, [](const Moves&, const State& state) {
      // If there is an immediately winning/losing move, we should have skipped the computation.
      assert(state.outcome == TIE);
      Outcome p = (*acc)[IndexOf(state.perm)];
      return p != LOSS;
    });
    return complete ? TIE : WIN;
  }
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

std::vector<uint8_t> ComputeChunk(int chunk) {
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
  return EncodeOutcomes(outcomes);
}

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

void InitPhase(int phase) {
  expected_outcome = phase % 2 == 0 ? WIN : LOSS;
  std::cout << "Expected outcome: " << OutcomeToString(expected_outcome) << "." << std::endl;

  acc.emplace(PhaseInputFilename(phase - 1).c_str());

  if (VerifyInputChunks(phase - 1, acc.value()) != 0) exit(1);
}

void RunManually(int phase, int start_chunk, int end_chunk) {
  std::cout << "Calculating " << end_chunk - start_chunk << " R" << phase << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive) "
      << "using " << num_threads << " threads." << std::endl;
  FOR(chunk, start_chunk, end_chunk) {
    const std::string filename = ChunkFileName(phase, "output", chunk);
    if (std::filesystem::exists(filename)) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    auto start_time = std::chrono::system_clock::now();
    std::vector<uint8_t> bytes = ComputeChunk(chunk);
    WriteToFile(filename, bytes);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Chunk " << chunk << " done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}

void PrintUsage() {
  std::cout << solver_id << "\n\n"
    << "For manual chunk assignment:\n\n"
    << "  solve-rN --phase=N --start=<start-chunk> --end=<end-chunk>\n\n"
    << "For automatic chunk assignment (requires network access):\n\n"
    << "  solve-rN --phase=N --user=<user-id> --machine=<machine-id>\n"
    << "      [--host=styx.verver.ch] [--port=7429]"
    << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  InitializePerms();

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
          return ChunkFileName(phase, "output", chunk);
        },
        ComputeChunk);
    solver.Run();
  }
}
