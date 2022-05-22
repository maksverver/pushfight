// Counts impossible permutations.
//
// A permutation is impossible if it can't be reached from any starting position
// by a valid sequence of moves. Although it is hard to calculate possibility
// 100% correctly, we can at least eliminate some impossible permutations where
// the anchor is on a piece that we know cannot have made a push move in the
// last turn. This tool counts how many such impossible permutations exist.

#include "board.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>

void Report(int64_t possible, int64_t total, std::ostream &os) {
  int64_t impossible = total - possible;
  os << "Processed " << total << " permutations. "
      << possible << " possible (" << std::fixed << std::setprecision(2)
          << (100.0 * possible / total) << "%). "
      << impossible << " impossible (" << std::fixed << std::setprecision(2)
          << (100.0 * impossible / total) << "%)."
      << std::endl;
}

int main() {
  InitializePerms();
  Perm perm = first_perm;
  int64_t index = 0;
  int64_t possible_count = 0;
  constexpr int reporting_interval = 1 << 30;
  auto start_time = std::chrono::system_clock::now();
  do {
    //std::cerr << index << " " << (IsReachable(perm) ? "possible" : "impossible") << std::endl;
    if (IsReachable(perm)) {
      ++possible_count;
    }
    ++index;
    if (index % reporting_interval == 0) {
      // Print progress as a percentage, and estimated time remaining in minutes.
      auto end_time = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end_time - start_time;
      start_time = end_time;
      double remaining_seconds = (total_perms - index) * elapsed_seconds.count() / reporting_interval;
      Report(possible_count, index, std::cerr);
      std::cerr << 100.0 * index / total_perms << "% complete. "
          << "Estimated time remaining: " << remaining_seconds / 60 << " minutes."
          << std::endl;
    }
  } while (std::next_permutation(perm.begin(), perm.end()));
  Report(possible_count, index, std::cout);
}
