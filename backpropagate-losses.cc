// Tool to backpropagate losses detected in phase 1 to wins in phase 2.
//
// The idea is to loop over all LOSSes, generate all predecessors, and mark them
// all as WINs.
//
// This may be faster than the forward search done by solve-rN, since there are
// significantly fewer LOSSes than remaining TIEs, while the number of
// predecessors of a position is similar to the number of successors.
//
// The downside is that this approach doesn't allow generating output in chunks,
// because predecessors may appear anywhere in the output.
//
// TODO: implement multithreading support? Note that we must be careful when
// writing back, because writing requires a read-modify-write cycle that cannot
// be done atomically!
//
// In theory the same approach can be used for later even phases (4, 6, 8, etc.)
// however, to avoid doing unnecessary work, we need to know which losses are
// new! So some additional diffing may be necessary, i.e. to calculate phase N
// (where N is even) we need to diff phase (N - 1) and (N - 3) to find new
// losses to backpropagate.

#include "accessors.h"
#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

MutableRnAccessor acc = MutableRnAccessor("input/r2.bin");

void ProcessChunk(int chunk) {
  int64_t losses_found = 0;
  int64_t wins_written = 0;
  int64_t total_predecessors = 0;
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  Perm perm = PermAtIndex(start_index);
  REP(i, chunk_size) {
    if (acc[i] == LOSS) {
      ++losses_found;
      GeneratePredecessors(
        perm,
        [&total_predecessors, &wins_written](const Perm &predecessor){
          ++total_predecessors;
          int64_t pred_index = IndexOf(predecessor);
          Outcome o = acc[pred_index];
          if (o == TIE) {
            // TODO: write!
            ++wins_written;
          } else {
            assert(o == WIN);
          }
        });
    }
    std::next_permutation(perm.begin(), perm.end());
  }
  std::cerr << "Chunk stats: "
      << losses_found << " losses found. "
      << wins_written << " wins written.\n";
  if (losses_found > 0) {
    std::cerr << "Average number of predecessors: " << total_predecessors / losses_found << ".\n";
  }
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
  const int start_chunk = argc > 1 ? std::max(0, ParseInt(argv[1])) : 0;
  const int end_chunk = argc > 2 ? std::min(ParseInt(argv[2]), num_chunks) : num_chunks;

  std::cout << "Backprogating losses in " << end_chunk - start_chunk << " chunks "
      << "from " << start_chunk << " to " << end_chunk << " (exclusive)." << std::endl;

  InitializePerms();
  FOR(chunk, start_chunk, end_chunk) {
    auto start_time = std::chrono::system_clock::now();
    ProcessChunk(chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Chunk " << chunk << " done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}
