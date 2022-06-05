// Tool to integrate the wins found with backpropagate2 into a ternary file.

#include <cassert>
#include <cstring>
#include <iostream>
#include <optional>

#include "accessors.h"
#include "efcodec.h"
#include "bytes.h"
#include "codec.h"
#include "macros.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: integrate-wins2 [--dry-run] <rN.bin> <chunk-rN-wins.bin...>\n";
    return 0;
  }

  bool dry_run = false;
  int start_argi = 1;
  if (strcmp(argv[start_argi], "--dry-run") == 0) {
    dry_run = true;
    ++start_argi;
  }

  MutableRnAccessor acc(argv[start_argi++]);
  int64_t total_perms = 0;
  int64_t total_changes = 0;
  for (int i = start_argi; i < argc; ++i) {
    const char *filename = argv[i];
    int64_t changes = 0;
    bytes_t bytes = ReadFromFile(filename);
    std::optional<std::vector<int64_t>> ints = DecodeEF(bytes);
    if (!ints) {
      std::cerr << "Failed to decode " << filename << std::endl;
      return 1;
    }
    for (int64_t i : *ints) {
      Outcome o = acc[i];
      if (o == WIN) {
        // Already marked winning.
        continue;
      }
      if (o == LOSS) {
        std::cerr << filename << ": Permutation " << i << " is already marked LOSS!" << std::endl;
        return 1;
      }
      assert(o == TIE);
      if (!dry_run) acc[i] = WIN;
      ++changes;
    }
    int64_t perms = ints->size();
    std::cout << filename << ": " << perms << " permutations, " << changes << " new wins recorded." << std::endl;
    total_perms += perms;
    total_changes += changes;
  }
  std::cout << "Total " << total_perms << " permutations, " << total_changes << " new wins recorded." << std::endl;
}
