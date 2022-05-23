#include "macros.h"

#include "board.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>

// Counts the contents of r1 chunks which encode a stream of ternary Outcome values.
int main(int argc, char* argv[]) {
  int64_t counts[3] = {0, 0, 0};
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
        }
        assert(byte == 0);
      }
    }
    assert(ifs.eof() && !ifs.bad());
  }
  int64_t total = counts[0] + counts[1] + counts[2];
  std::cout << "Ties:   " << counts[TIE] << std::endl;
  std::cout << "Losses: " << counts[LOSS] << std::endl;
  std::cout << "Wins:   " << counts[WIN] << std::endl;
  std::cout << "Total:  " << total << std::endl;
}
