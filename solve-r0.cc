#include "board.h"
#include "chunks.h"
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
int num_threads = std::thread::hardware_concurrency();

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

void ComputeChunkThread(int chunk, std::atomic<int> *next_part, Outcome outcomes[]) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= num_threads) PrintChunkUpdate(chunk, part + 1 - num_threads);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    Perm perm = PermAtIndex(start_index + part_start);
    REP(i, part_size) {
      outcomes[part_start + i] = HasWinningMove(perm) ? WIN : TIE;
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

int ParseInt(const char *s) {
  int i = -1;
  std::istringstream iss(s);
  iss >> i;
  assert(iss);
  return i;
}

}  // namespace

int main(int argc, char *argv[]) {
  int start_chunk = argc > 1 ? std::max(0, ParseInt(argv[1])) : 0;
  int end_chunk = argc > 2 ? std::min(ParseInt(argv[2]), num_chunks) : num_chunks;

  std::cout << "Calculating " << end_chunk - start_chunk << " chunks from " << start_chunk << " to "
      << end_chunk << " (exclusive) using " << num_threads << " threads." << std::endl;

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
