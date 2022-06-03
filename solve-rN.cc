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

#include "client/client.h"
#include "accessors.h"
#include "board.h"
#include "bytes.h"
#include "codec.h"
#include "chunks.h"
#include "flags.h"
#include "hash.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

// This class has friend access to RnAccessor to be able to access the
// underlying memory mapped file.
class ChunkVerifier {
public:
  constexpr static size_t chunk_byte_size = chunk_size / 5;

  ChunkVerifier(const RnAccessor &acc) : acc(acc) {}

  sha256_hash_t ComputeChunkHash(int chunk) {
    return ComputeSha256(GetChunkBytes(chunk));
  }

private:
  byte_span_t GetChunkBytes(int chunk) {
    return byte_span_t(acc.map.data() + chunk_byte_size * chunk, chunk_byte_size);
  }

  const RnAccessor &acc;
};

namespace {

const std::string solver_id = "solve-rN-v0.1.0";

// These are SHA256 sums of r4.bin (the input to phase 5).
constexpr std::pair<int, const char *> phase_5_chunk_hashes[] = {
  // First three chunks
  {0, "e56a04df6ec6e61b03e651247929c8e99048c87274d0582abfb868cf7ba10fe4"},
  {1, "440024667100b0aed051b067ed089ccb90b82b2bc24f9e0fa6d7f1eb7e1f6fe6"},
  {2, "66b67cf63717a57cabe3ce9f1a3ecdea8383feb8a43a6dc39e05f77098963dbe"},

  // Last three chunks
  {7426, "e29eb5ad29401478d4170e92cf3728312672060c72e6d12d813dbb7d8c8f4306"},
  {7427, "118f46577b86d2623363ee1f076854bc064e6700288cdc9e22ff25974111705f"},
  {7428, "49e6d3ffd64e5bf03ad08fae0299cf0f889458a2c245316722deed534feb0243"},

  // Chunks that were wrongly calculated in an old version of r4.bin :-(
  {2436, "66a4effeb5b08bd0a943095b721c55a323d6c808b2f3a7981f5d0a16482a42c9"},
  {2500, "9c23655ae2783e7fd83a609b6bb611765c776942acaba516bd93f5039f2f72c7"},
  {3603, "ab109a5e233114d49ca0110f5769c5f5107d14b5ac063d394683e733507b951b"},
  {4898, "a2e7804978dc9048b60d5acb41eece77dfe0766512bbedd358b409c111a55bf9"},
  {5824, "1d203d24cc6fb9ffbc606678b321740418293e2bb7a1da141e8bec86287eeae6"},
};

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

std::vector<uint8_t> ReadFromFile(const std::string &filename) {
  size_t size = std::filesystem::file_size(filename);
  std::vector<uint8_t> bytes(size);
  std::ifstream ifs(filename, std::ofstream::binary);
  if (!ifs) {
    std::cerr << "Could not open input file: " << filename << std::endl;
    exit(1);
  }
  ifs.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  assert(ifs);
  assert(ifs.gcount() == bytes.size());
  return bytes;
}

void WriteToFile(const std::string &filename, const std::vector<uint8_t> &bytes) {
  std::ofstream ofs(filename, std::ofstream::binary);
  if (!ofs) {
    std::cerr << "Could not open output file: " << filename << std::endl;
    exit(1);
  }
  ofs.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  assert(ofs);
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

  if (phase == 5) {
    // Verify the input file (r4.bin) is correct by checking some chunk hashes.
    ChunkVerifier verifier = ChunkVerifier(acc.value());
    int failures = 0;
    for (const auto &pair : phase_5_chunk_hashes) {
      int chunk = pair.first;
      std::optional<sha256_hash_t> expected_hash = HexDecode(pair.second);
      if (!expected_hash) {
        std::cerr << "Couldn't decode expected hash for chunk " << chunk << "!" << std::endl;
        ++failures;
      } else {
        sha256_hash_t computed_hash = verifier.ComputeChunkHash(chunk);
        if (computed_hash != *expected_hash) {
          std::cerr << "Verification of chunk " << chunk << " failed!\n"
              << "Expected SHA-256 sum: " << HexEncode(*expected_hash) << "\n"
              << "Computed SHA-256 sum: " << HexEncode(computed_hash) << std::endl;
          ++failures;
        }
      }
    }
    if (failures != 0) exit(1);
  }
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
    std::vector<Outcome> outcomes = ComputeChunk(chunk);
    std::vector<uint8_t> bytes = EncodeOutcomes(outcomes);
    WriteToFile(filename, bytes);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Chunk " << chunk << " done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}

class AutomaticSolver {
public:
  static constexpr int min_sleep_seconds = 5;
  static constexpr int max_sleep_seconds = 600;  // 10 minutes

  AutomaticSolver(int phase, std::string host, std::string port, std::string user, std::string machine)
    : phase(phase), host(std::move(host)), port(std::move(port)),
      user(std::move(user)), machine(std::move(machine)) {
  }

  void Run() {
    for (;;) {
      if (chunks.empty()) {
        if (GetMoreChunks()) {
          ResetSleepTime();
        } else {
          Sleep();
        }
      } else {
        int chunk = chunks.front();
        chunks.pop_front();
        ProcessChunk(chunk);
      }
    }
  }

private:

  ErrorOr<Client> Connect() {
    return Client::Connect(phase, host.c_str(), port.c_str(), solver_id, user, machine);
  }

  void ResetSleepTime() {
    sleep_seconds = 0;
  }

  void Sleep() {
    if (sleep_seconds == 0) {
      sleep_seconds = min_sleep_seconds;
    } else {
      sleep_seconds = std::min(sleep_seconds * 2, max_sleep_seconds);
    }
    std::cout << "Sleeping for " << sleep_seconds << " seconds before retrying..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
  }

  bool GetMoreChunks() {
    std::cout << "Queue is empty. Fetching more chunks from the server at "
        << host << ":" << port << "..." << std::endl;
    if (auto client = Connect(); !client) {
      std::cerr << "Failed to connect: " << client.Error().message << std::endl;
    } else if (auto chunks = client->GetChunks(); !chunks) {
      std::cerr << "Failed to get chunks: " << chunks.Error().message << std::endl;
    } else if (chunks->empty()) {
      std::cerr << "Server has no more chunks available!" << std::endl;
    } else {
      std::cout << "Server returned " << chunks->size() << " more chunks to solve." << std::endl;
      this->chunks.insert(this->chunks.end(), chunks->begin(), chunks->end());
      return true;
    }
    return false;
  }

  void ProcessChunk(int chunk) {
    const std::string filename = ChunkFileName(phase, "output", chunk);
    std::vector<uint8_t> bytes;
    if (std::filesystem::exists(filename) && std::filesystem::file_size(filename) > 0) {
      std::cout << "Chunk output already exists. Loading..." << std::endl;
      bytes = ReadFromFile(filename);
      assert(!bytes.empty());
    } else {
      std::cout << "Calculating chunk " << chunk
          << " using " << num_threads << " threads." << std::endl;
      bytes = EncodeOutcomes(ComputeChunk(chunk));
      WriteToFile(filename, bytes);
    }
    std::cout << "Chunk complete! Reporting result to server..." << std::endl;
    ReportChunk(chunk, bytes);
  }

  bool ReportChunk(int chunk, const std::vector<uint8_t> &bytes) {
    if (auto client = Connect(); !client) {
      std::cerr << "Failed to connect: " << client.Error().message << std::endl;
      return false;
    } else if (ErrorOr<size_t> uploaded = client->SendChunk(chunk, bytes); !uploaded) {
      std::cout << "Failed to send result to server: " << uploaded.Error().message << std::endl;
      return false;
    } else if (*uploaded == 0) {
      std::cout << "Succesfully reported result to server! (No upload required.)" << std::endl;
      return true;
    } else {
      std::cout << "Succesfully uploaded chunk to server! "
          << "(" << bytes.size() << " bytes; " << *uploaded << " bytes compressed)" << std::endl;
      return true;
    }
  }

  int phase = -1;
  std::string host, port, user, machine;
  std::deque<int> chunks;
  int sleep_seconds = 0;
};

void PrintUsage() {
  std::cout << "Usage:\n\n"
    << "For manual chunk assignment:\n\n"
    << "  solve-rN --phase=N --user=<user-id> --machine=<machine-id>\n\n"
    << "For automatic chunk assignment (requires network access):\n\n"
    << "  solve-rN --phase=N --start=<start-chunk> --end=<end-chunk>\n"
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
    AutomaticSolver solver(phase, arg_host, arg_port, arg_user, arg_machine);
    solver.Run();
  }
}
