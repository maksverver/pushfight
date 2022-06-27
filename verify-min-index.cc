// Verifies that there is a 1-to-1 mapping between reachable permutations
// and minimal indices.
//
// Requires a lot of RAM (~20 GB) to run!

#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <thread>

// Number of threads to use for calculations. 0 to disable multithreading.
int num_threads = std::thread::hardware_concurrency();

struct MinIndexBits {
  static_assert(min_index_size % 8 == 0);
  std::unique_ptr<std::atomic<uint8_t>[]> seen =
      std::make_unique<std::atomic<uint8_t>[]>(min_index_size / 8);

  bool Get(int64_t i) const {
    assert(i >= 0);
    uint64_t u = i;
    return (seen[u / 8] >> (u % 8)) & 1;
  }

  bool Set(int64_t i)  {
    assert(i >= 0);
    uint64_t u = i;
    uint8_t old_byte = seen[u / 8].fetch_or(1 << (u % 8));
    return (old_byte >> (u % 8)) & 1;
  }
};

static MinIndexBits seen1;  // normal
static MinIndexBits seen2;  // rotated

static std::atomic<int> duplicates = 0;

static void CountReachableThread(std::atomic<int> *next_chunk) {
  for (;;) {
    int chunk = (*next_chunk)++;
    if (chunk >= num_chunks) break;
    int64_t index = int64_t{chunk} * int64_t{chunk_size};
    Perm perm = PermAtIndex(index);
    REP(i, chunk_size) {
      if (i > 0) {
        ++index;
        bool advance = std::next_permutation(perm.begin(), perm.end());
        assert(advance);
      }
      if (IsReachable(perm)) {
        bool rotated = false;
        int64_t min_index = MinIndexOf(perm, &rotated);
        if ((rotated ? seen2 : seen1).Set(min_index)) {
          ++duplicates;
          std::cerr << "Duplicate min-index " << min_index
              << (rotated ? " rotated " : " normal ")
              << "index " << index << std::endl;
        }
      }
    }
    std::cerr << "Chunk " << chunk << " / " << num_chunks << " done." << std::endl;
  }
}

int main() {
  InitializePerms();

  std::atomic<int> next_chunk = 0;
  std::vector<std::thread> threads;
  threads.reserve(num_threads);
  REP(i, num_threads) {
    threads.emplace_back(CountReachableThread, &next_chunk);
  }
  REP(i, num_threads) {
    threads[i].join();
  }

  if (duplicates) {
    std::cerr << duplicates << " duplicates!" << std::endl;
    return 1;
  }

  // Verify that all minimized indices were found.
  int64_t missing = 0;
  for (size_t i = 0; i < min_index_size; ++i) {
    if (!seen1.Get(i)) {
      std::cerr << "Missing min-index " << i << " (normal)" << std::endl;
      ++missing;
    }
    if (!seen2.Get(i)) {
      std::cerr << "Missing min-index " << i << " (rotated)" << std::endl;
      ++missing;
    }
  }
  if (missing) {
    std::cerr << missing << " missing values!" << std::endl;
    return 1;
  }

  return 0;
}
