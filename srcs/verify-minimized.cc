// Verifies the correctness of minimized.bin completely.
//
// Sadly, this is quite slow. (Might take in the order of 100 days to verify
// everything.)

#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "minimized-accessor.h"
#include "minimized-lookup.h"
#include "parse-int.h"
#include "perms.h"
#include "position-value.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
const int thread_count = std::thread::hardware_concurrency();

// Createa checkpoint after this many iterations.
const int64_t checkpoint_interval = 100000;

void VerifyThread(
    MinimizedAccessor *acc,
    int64_t end_index,
    std::atomic<int64_t> *next_index,
    std::atomic<int64_t> *failures,
    std::mutex *io_mutex) {
  int64_t index;
  while ((index = (*next_index)++) < end_index) {
    Perm perm = PermAtMinIndex(index);
    uint8_t expected_byte = RecalculateValue(*acc, perm).byte;
    uint8_t actual_byte = acc->ReadByte(index);
    if (expected_byte != actual_byte) [[unlikely]] {
      std::lock_guard lock(*io_mutex);
      std::cerr << "FAILURE at min-index +" << index << "! "
          << "expected: " << int{expected_byte} << " (" << Value(expected_byte) << "); "
          << "actual: " << int{actual_byte} << " (" << Value(actual_byte) << ")." << std::endl;
      ++(*failures);
    }
  }
}

int64_t Verify(MinimizedAccessor &acc, int64_t start_index, int64_t end_index) {
  std::atomic<int64_t> next_index = start_index;
  std::atomic<int64_t> failures = 0;
  std::mutex io_mutex;
  if (thread_count == 0) {
    VerifyThread(&acc, end_index, &next_index, &failures, &io_mutex);
    assert(next_index == end_index + 1);
  } else {
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    REP(i, thread_count) {
      threads.emplace_back(
          VerifyThread, &acc, end_index, &next_index, &failures, &io_mutex);
    }
    REP(i, thread_count) {
      threads[i].join();
    }
    assert(next_index == end_index + thread_count);
  }
  return failures;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 5) {
    std::cerr << "Usage: expand-minimized <minimized.bin> <checkpoint-file> [<start-index> [<end-index>]]" << std::endl;
    return EXIT_FAILURE;
  }

  const char *minimized_path = argv[1];
  const char *checkpoint_path = argv[2];

  int64_t start_index = 0;
  int64_t end_index = min_index_size;
  if (argc > 3) {
    start_index = ParseInt64(argv[3]);
    if (start_index < 0 || start_index >= min_index_size) {
      std::cerr << "Invalid start index: " << argv[3] << std::endl;
      return EXIT_FAILURE;
    }
    if (argc > 4) {
      end_index = ParseInt64(argv[4]);
      if (end_index < start_index || end_index > min_index_size) {
        std::cerr << "Invalid end index: " << argv[4] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  int64_t checkpoint_index = 0;
  std::ifstream ifs(checkpoint_path);
  if (!ifs || !(ifs >> checkpoint_index)) {
    std::cerr << "Could not read previous checkpoint index! "
        << "Restarting computation from index " << start_index << "." << std::endl;
    checkpoint_index = start_index;
  } else if (checkpoint_index < start_index || checkpoint_index > end_index) {
    std::cerr << "Checkpoint index (" << checkpoint_index << ") out of range!" << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cerr << "Resuming from checkpoint index " << checkpoint_index << std::endl;
  }

  MinimizedAccessor acc(minimized_path);

  while (checkpoint_index < end_index) {
    int64_t chunk_start_index = checkpoint_index;
    int64_t chunk_end_index = std::min(checkpoint_index + checkpoint_interval, end_index);
    int64_t failures = Verify(acc, chunk_start_index, chunk_end_index);
    if (failures != 0) {
      std::cerr << "Verification failures detected!" << std::endl;
      return EXIT_FAILURE;
    }
    checkpoint_index = chunk_end_index;
    std::ofstream ofs(checkpoint_path);
    if (!ofs || !(ofs << checkpoint_index << std::endl)) {
      std::cerr << "Failed to write checkpoint index!";
      return EXIT_FAILURE;
    }
    double percent_complete = 100.0 * (checkpoint_index - start_index) / (end_index - start_index);
    std::cerr << "Wrote checkpoint at index " << checkpoint_index
        << " (" << std::fixed << percent_complete << "% done)" << std::endl;
  }
  std::cerr << "Verification complete!" << std::endl;
  return EXIT_SUCCESS;
}
