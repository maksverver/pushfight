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

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
const int thread_count = std::thread::hardware_concurrency();

void VerifyThread(
    MinimizedAccessor *acc,
    int64_t start_index,
    int64_t end_index,
    std::atomic<int64_t> *next_index,
    std::atomic<int64_t> *failures,
    std::mutex *io_mutex) {
  int64_t index;
  while ((index = (*next_index)++) < end_index) {
    Perm perm = PermAtMinIndex(index);
    uint8_t expected_byte = RecalculateValue(*acc, perm).byte;
    uint8_t actual_byte = acc->ReadByte(index);
    if (expected_byte != actual_byte) {
      std::lock_guard lock(*io_mutex);
      std::cerr << "\rFAILURE at min-index +" << index << "! "
          << "expected: " << int{expected_byte} << " (" << Value(expected_byte) << "); "
          << "actual: " << int{actual_byte} << " (" << Value(actual_byte) << ")." << std::endl;
      ++(*failures);
    }
    if (index % 1000 == 0) {
      std::lock_guard lock(*io_mutex);
      std::cerr << "\rVerified index " << index << " (" << std::fixed << std::setprecision(2)
          << 100.0 * (index - start_index) / (end_index - start_index) << "% done)" << std::endl;
    }
  }
}

int64_t Verify(MinimizedAccessor &acc, int64_t start_index, int64_t end_index) {
  std::atomic<int64_t> next_index = start_index;
  std::atomic<int64_t> failures = 0;
  std::mutex io_mutex;
  if (thread_count == 0) {
    VerifyThread(&acc, start_index, end_index, &next_index, &failures, &io_mutex);
    assert(next_index == end_index + 1);
  } else {
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    REP(i, thread_count) {
      threads.emplace_back(
          VerifyThread, &acc, start_index, end_index, &next_index, &failures, &io_mutex);
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
  if (argc < 2 || argc > 4) {
    std::cerr << "Usage: expand-minimized <minimized.bin> [<start-index> [<end-index>]]" << std::endl;
    return 1;
  }

  int64_t start_index = 0;
  int64_t end_index = min_index_size;
  if (argc > 2) {
    start_index = ParseInt64(argv[2]);
    if (start_index < 0 || start_index >= min_index_size) {
      std::cerr << "Invalid start index: " << argv[2] << std::endl;
      return 1;
    }
    if (argc > 3) {
      end_index = ParseInt64(argv[3]);
      if (end_index < start_index || end_index > min_index_size) {
        std::cerr << "Invalid end index: " << argv[3] << std::endl;
        return 1;
      }
    }
  }

  MinimizedAccessor acc(argv[1]);
  int64_t failures = Verify(acc, start_index, end_index);
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
