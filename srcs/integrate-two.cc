// Tool to integrate the losses and wins found with solver2 into a ternary file.

#include <cassert>
#include <cstring>
#include <iostream>
#include <optional>

#include "accessors.h"
#include "efcodec.h"
#include "bytes.h"
#include "codec.h"
#include "macros.h"

template<class Acc>
void Assign(Acc &acc, int64_t i, Outcome o) {
  acc[i] = o;
}

template<>
void Assign<RnAccessor>(RnAccessor &, int64_t, Outcome) {
  // Read only accessor. Do nothing.
}

template<class Acc>
int Main(Acc &acc, int start_argi, int argc, char *argv[]) {
  int64_t total_losses = 0;
  int64_t total_wins = 0;
  int64_t total_new_losses = 0;
  int64_t total_new_wins = 0;
  FOR(i, start_argi, argc) {
    const char *filename = argv[i];
    bytes_t bytes = ReadFromFile(filename);
    byte_span_t byte_span = bytes;
    REP(p, 2) {
      int64_t changes = 0;
      const char *what = p == 0 ? "losses" : "wins";
      Outcome new_outcome = p == 0 ? LOSS : WIN;
      std::optional<std::vector<int64_t>> ints = DecodeEF(&byte_span);
      if (!ints) {
        std::cerr << "Failed to decode " << what << " in file: " << filename << std::endl;
        return 1;
      }
      for (int64_t i : *ints) {
        Outcome o = acc[i];
        if (o == new_outcome) {
          continue;
        }
        if (o != TIE) {
          std::cerr << filename << ": Permutation " << i << " is marked " << OutcomeToString(o)
              << " should be " << OutcomeToString(new_outcome) << std::endl;
          return 1;
        }
        Assign<Acc>(acc, i, new_outcome);
        ++changes;
      }
      int64_t perms = ints->size();
      std::cout << filename << ": " << perms << " " << what << ", " << changes << " new " << what << "." << std::endl;
      if (p == 0) {
        total_losses += perms;
        total_new_losses += changes;
      } else {
        total_wins += perms;
        total_new_wins += changes;
      }
    }
  }
  std::cout << "Total " << total_losses + total_wins << " permutations, "
      << total_losses << " losses, "
      << total_new_losses << " new losses, "
      << total_wins << " wins, "
      << total_new_wins << " new wins." << std::endl;
  return 0;

}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Usage: integrate-two [--dry-run] <rN.bin> <chunk-rN-two.bin...>\n";
    return 0;
  }

  int start_argi = 1;
  bool dry_run = false;
  if (strcmp(argv[start_argi], "--dry-run") == 0) {
    dry_run = true;
    ++start_argi;
  }

  if (dry_run) {
    RnAccessor acc(argv[start_argi++]);
    Main<RnAccessor>(acc, start_argi, argc, argv);
  } else {
    MutableRnAccessor acc(argv[start_argi++]);
    Main<MutableRnAccessor>(acc, start_argi, argc, argv);
  }
}
