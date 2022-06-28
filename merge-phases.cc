// Tool to merge results from phases into a single output file.
//
// The output file has 1 byte per permutation that describes whether the
// position is won, lost and tied, and in how many moves. This can be used to
// implement an optimal strategy.
//
// Inputs
//
//  r0.bin (binary!)
//  r2.bin (ternary)
//  r4.bin (ternary)
//  r6.bin (ternary)
//  ..
//  r18.bin (ternary)
//  r20-new.bin (ef-coded)
//  r22-new.bin (ef-coded)
//  r24-new.bin (ef-coded)
//  ..
//
// Output file format
//
//  0: tied
//  1: loss in 0  (from lost-positions.cc)
//  2: win in 1   (from r0)
//  3: loss in 1  (from r2)
//  4: win in 2   (from r2)
//  5: loss in 2  (from r4)
//  6: win in 3   (from r4)
// ..
// 19: loss in 9  (from r18)
// 20: win in 10  (from r18)
// 21: loss in 10 (from r20-new)
// 22: win in 11  (from r20-new)
// 23: loss in 11 (from r22-new)
// 24: win in 22  (from r22-new)
//  etc.
//

#include "board.h"
#include "codec.h"
#include "chunks.h"
#include "efcodec.h"
#include "macros.h"
#include "lost-positions.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <memory>

namespace {

constexpr int full_input_count = 10;

Outcome ValueToOutcome(int value) {
  return value == 0 ? TIE : (value % 2 == 1) ? LOSS : WIN;
}

}  // namespace

int main(int argc, char *argv[]) {
  const int64_t *imm_lost_pos = ImmediatelyLostBegin();
  const int64_t *imm_lost_end = ImmediatelyLostEnd();

  if (argc < full_input_count + 1) {
    std::cerr << "Not enough arguments." << std::endl;
    return 1;
  }
  int diff_count = argc - 1 - full_input_count;

  std::vector<std::unique_ptr<std::ifstream>> files;
  FOR(i, 1, argc) {
    auto p = std::make_unique<std::ifstream>(argv[i], std::istream::binary);
    if (!*p) {
      std::cerr << "Failed to open argument " << i << " (" << argv[i] << ")" << std::endl;
      return 1;
    }
    files.push_back(std::move(p));
  }

  BinaryReader r0(*files[0]);
  std::vector<std::unique_ptr<TernaryReader>> rN;
  FOR(i, 1, full_input_count) {
    rN.push_back(std::make_unique<TernaryReader>(*files[i]));
  }

  REP(chunk, num_chunks) {
    std::vector<std::vector<int64_t>> new_wins;
    std::vector<std::vector<int64_t>> new_losses;
    std::vector<size_t> new_win_pos;
    std::vector<size_t> new_loss_pos;
    REP(i, diff_count) {
      std::istream &is = *files[full_input_count + i];
      new_losses.push_back(DecodeEF(is).value());
      new_wins.push_back(DecodeEF(is).value());
      new_loss_pos.push_back(0);
      new_win_pos.push_back(0);
    }

    std::vector<int> freq;
    std::vector<uint8_t> chunk_ouput(chunk_size);
    REP(i, chunk_size) {
      int64_t index = int64_t{chunk} * int64_t{chunk_size} + int64_t{i};
      int value = 0;
      if (imm_lost_pos < imm_lost_end && *imm_lost_pos <= index) {
        assert(*imm_lost_pos == index);
        value = 1;
        ++imm_lost_pos;
      }

      bool o0 = r0.Next();
      if (o0) {
        assert(value == 0);
        value = 2;
      }

      REP(i, rN.size()) {
        Outcome o = rN[i]->Next();
        if (o == TIE) {
          assert(value == 0);
        } else if (value == 0) {
          value = o == LOSS ? 2*i + 3 : 2*i + 4;
        } else {
          assert(ValueToOutcome(value) == o);
        }
      }

      REP(i, diff_count) {
        if (new_loss_pos[i] < new_losses[i].size() && new_losses[i][new_loss_pos[i]] <= index) {
          assert(new_losses[i][new_loss_pos[i]] == index);
          assert(value == 0);
          value = 2*(full_input_count + i) + 1;
          ++new_loss_pos[i];
        }
        if (new_win_pos[i] < new_wins[i].size() && new_wins[i][new_win_pos[i]] <= index) {
          assert(new_wins[i][new_win_pos[i]] <= index);
          assert(value == 0);
          value = 2*(full_input_count + i) + 2;
          ++new_win_pos[i];
        }
      }

      assert(value >= 0 && value < 256);
      chunk_ouput[i] = value;

      if (value >= freq.size()) freq.resize(value + 1);
      ++freq[value];
    }
    REP(i, diff_count) {
      assert(new_loss_pos[i] == new_losses[i].size());
      assert(new_win_pos[i] == new_wins[i].size());
    }

    // Debug-print frequencies for comparison against results/expected-merged-frequencies.txt
    REP(i, freq.size()) if (freq[i] > 0) {
      std::cerr << chunk << ' '  << i << ' ' << freq[i] << '\n';
    }

    int total = 0;
    for (int i : freq) total += i;
    assert(total == 54054000);

    // TODO: write output
  }
  assert(imm_lost_pos == imm_lost_end);
}
