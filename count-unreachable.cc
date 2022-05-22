// Counts unreachable permutations.
//
// Some permutations are provably unreachable from any starting position by
// a valid sequence of moves. Although it is hard to calculate reachability
// 100% correctly, we can at least eliminate some unreachable permutations where
// the anchor is on a piece that we know cannot have made a push move in the
// last turn. This tool counts how many such unreachable permutations exist.

#include "board.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>

void Report(int64_t reachable, int64_t total, std::ostream &os) {
  int64_t unreachable = total - reachable;
  os << "Processed " << total << " permutations. "
      << reachable << " reachable (" << std::fixed << std::setprecision(2)
          << (100.0 * reachable / total) << "%). "
      << unreachable << " unreachable (" << std::fixed << std::setprecision(2)
          << (100.0 * unreachable / total) << "%)."
      << std::endl;
}

int main() {
  InitializePerms();
  Perm perm = first_perm;
  int64_t index = 0;
  int64_t reachable_count = 0;
  constexpr int reporting_interval = 1 << 30;
  auto start_time = std::chrono::system_clock::now();
  do {
    //std::cerr << index << " " << (IsReachable(perm) ? "reachable" : "unreachable") << std::endl;
    if (IsReachable(perm)) {
      ++reachable_count;
    }
    ++index;
    if (index % reporting_interval == 0) {
      // Print progress as a percentage, and estimated time remaining in minutes.
      auto end_time = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end_time - start_time;
      start_time = end_time;
      double remaining_seconds = (total_perms - index) * elapsed_seconds.count() / reporting_interval;
      Report(reachable_count, index, std::cerr);
      std::cerr << 100.0 * index / total_perms << "% complete. "
          << "Estimated time remaining: " << remaining_seconds / 60 << " minutes."
          << std::endl;
    }
  } while (std::next_permutation(perm.begin(), perm.end()));
  Report(reachable_count, index, std::cout);
}
