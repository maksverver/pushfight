// Tool to integrate the wins found with backpropagate-losses into a
// ternary file.

#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "accessors.h"
#include "codec.h"
#include "macros.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "Usage: integrate-wins <r(N-1).bin> <rN-wins.bin> <rN.bin>\n";
    return 0;
  }

  RnAccessor prev_acc(argv[1]);

  LossPropagationAccessor wins_acc(argv[2]);
  REP(i, num_chunks) if (!wins_acc.IsChunkComplete(i)) {
    std::cout << "Missing chunk " << i << std::endl;
    return 1;
  }

  if (std::filesystem::exists(argv[3])) {
    std::cout << "Output file " << argv[3] << " already exists! Not overwriting." << std::endl;
    return 1;
  }

  std::ofstream ofs(argv[3], std::ofstream::binary);
  std::vector<Outcome> outcomes;
  int chunk = 0;
  int64_t new_wins = 0, existing_wins = 0;
  for (int64_t i = 0; i < total_perms; ++i) {
    Outcome o = prev_acc[i];
    if (wins_acc.IsWinning(i)) {
      assert(o != LOSS);
      if (o == TIE) {
        ++new_wins;
        o = WIN;
      } else {
        assert(o == WIN);
        ++existing_wins;
      }
    }
    outcomes.push_back(o);
    if (outcomes.size() == chunk_size) {
      std::vector<uint8_t> bytes = EncodeOutcomes(outcomes);
      outcomes.clear();
      ofs.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
      std::cerr << "Chunk " << chunk << " / " << num_chunks << " written." << std::endl;
      ++chunk;
    }
  }
  std::cerr << "New wins: " << new_wins << std::endl;
  // Existing wins is expected to be 0 since backpropagate-losses doesn't mark them.
  std::cerr << "Existing wins: " << existing_wins << std::endl;
  assert(outcomes.empty());
  assert(chunk == num_chunks);
}
