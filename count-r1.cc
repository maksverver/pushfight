#include "macros.h"

#include "board.h"
#include "chunks.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>

static void PrintHeader() {
  std::cout << std::setw(6) << "Chunk"
    << ' ' << std::setw(12) << "Ties"
    << ' ' << std::setw(12) << "Losses"
    << ' ' << std::setw(12) << "Wins"
    << ' ' << std::setw(12) << "Total" << std::endl;
}

static void PrintRow(int chunk, const int64_t (&counts)[3]) {
  int64_t total = counts[0] + counts[1] + counts[2];
  std::cout << std::setw(6) << chunk
    << ' ' << std::setw(12) << counts[TIE]
    << ' ' << std::setw(12) << counts[LOSS]
    << ' ' << std::setw(12) << counts[WIN]
    << ' ' << std::setw(12) << total << std::endl;
}

// Counts the contents of r1 chunks which encode a stream of ternary Outcome values.
int main(int argc, char* argv[]) {
  PrintHeader();
  int64_t counts[3] = {0, 0, 0};
  int64_t chunk_counts[3] = {0, 0, 0};
  int chunk_pos = 0;
  int chunk_index = 0;
  for (int i = 1; i < argc; ++i) {
    std::ifstream ifs(argv[i], std::ifstream::binary);
    while (ifs) {
      uint8_t buffer[409600];
      ifs.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
      for (int n = ifs.gcount(), j = 0; j < n; ++j) {
        uint8_t byte = buffer[j];
        REP(_, 5) {
          int i = byte % 3;
          byte /= 3;
          ++counts[i];
          ++chunk_counts[i];
        }
        assert(byte == 0);
        if (++chunk_pos == chunk_size / 5) {
          // Print chunk summary, and reset counts.
          PrintRow(chunk_index, chunk_counts);
          chunk_counts[0] = chunk_counts[1] = chunk_counts[2] = 0;
          chunk_pos = 0;
          ++chunk_index;
        }
      }
    }
    assert(ifs.eof() && !ifs.bad());
  }
  PrintRow(-1, counts);
}
