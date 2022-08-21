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

#include <iostream>

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
  int64_t failures = 0;
  for (int64_t index = start_index; index < end_index; ++index) {
    Perm perm = PermAtMinIndex(index);
    uint8_t expected_byte = RecalculateValue(acc, perm).byte;
    uint8_t actual_byte = acc.ReadByte(index);
    if (expected_byte != actual_byte) {
      std::cerr << "FAILURE at min-index +" << index << "! "
          << "expected: " << int{expected_byte} << " (" << Value(expected_byte) << "); "
          << "actual: " << int{actual_byte} << " (" << Value(actual_byte) << ")." << std::endl;
      ++failures;
    }
  }
  return failures == 0 ? 0 : 1;
}
