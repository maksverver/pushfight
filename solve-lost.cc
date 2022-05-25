// This solver tries to find positions that are losing because the next player
// has no push without pushing his own pieces off the board.
//
// My hypothesis is that no such positions exist.

#include "board.h"
#include "perms.h"
#include "search.h"

#include <algorithm>
#include <chrono>
#include <iostream>

int main() {
  InitializePerms();
  Perm perm = first_perm;
  int64_t num_found = 0;
  int64_t index = 0;
  constexpr int reporting_interval = 1 << 30;
  auto start_time = std::chrono::system_clock::now();
  do {
    bool complete = GenerateSuccessors(perm, [](const Moves &moves, const State &state) {
      (void) moves;  // unused
      // Skip states where player pushes their own piece off the board.
      // Abort the search in any other case.
      return state.outcome == WIN;
    });
    if (complete) {
      ++num_found;
      std::cout << "Found losing permutation: " << index << std::endl;
      std::cout << perm << std::endl;
    }
    ++index;
    if (index % reporting_interval == 0) {
      // Print progress as a percentage, and estimated time remaining in minutes.
      auto end_time = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsed_seconds = end_time - start_time;
      start_time = end_time;
      double remaining_seconds = (total_perms - index) * elapsed_seconds.count() / reporting_interval;
      std::cerr << 100.0 * index / total_perms << "% complete. "
          << "Estimated time remaining: " << remaining_seconds / 60 << " minutes."
          << std::endl;
    }
  } while (std::next_permutation(perm.begin(), perm.end()));
  std::cout << num_found << " / " << index << " permutations are losing. " << std::endl;
}
