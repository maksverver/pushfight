// Converts minimized.bin (~86 GB) to merged.bin (~410 GB).
//
// For reachable positions, this copies the relevant bytes from minimized.bin,
// and for unreachable positions (which are not represented in minimized.bin),
// it uses the logic from lookup-min to reconstruct the value.
//
// The merged output is written to standard output.
//
// Note that it's better to use the decompressed minimized.bin or progress will
// be very slow. Even then, this tool is pretty slow, taking over an hour to
// reconstruct one chunk (54054000 positions).

#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "minimized-accessor.h"
#include "minimized-lookup.h"
#include "parse-int.h"
#include "perms.h"
#include "position-value.h"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    std::cerr << "Usage: expand-minimized <minimized.bin> [<start-index> [<end-index>]]" << std::endl;
    return 1;
  }

  int64_t start_index = 0;
  int64_t end_index = total_perms;
  if (argc > 2) {
    start_index = ParseInt64(argv[2]);
    if (start_index < 0 || start_index >= total_perms) {
      std::cerr << "Invalid start index: " << argv[2] << std::endl;
      return 1;
    }
    if (argc > 3) {
      end_index = ParseInt64(argv[3]);
      if (end_index < start_index || end_index > total_perms) {
        std::cerr << "Invalid end index: " << argv[3] << std::endl;
        return 1;
      }
    }
  }

  MinimizedAccessor acc(argv[1]);
  Perm perm = PermAtIndex(start_index);
  for (int64_t index = start_index; index < end_index; ++index) {
    std::cout << LookupValue(acc, perm, nullptr).value().byte;
    std::next_permutation(perm.begin(), perm.end());
  }
}
