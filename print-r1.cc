#include "macros.h"

#include "board.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <fstream>

// Prints the contents of r1 chunks which encode a stream of ternary Outcome values.
int main(int argc, char* argv[]) {
  int64_t counts[3] = {0, 0, 0};
  int64_t byte_index = 0;
  for (int i = 1; i < argc; ++i) {
    std::ifstream ifs(argv[i]);
    while (ifs) {
      uint8_t buffer[409600];
      ifs.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
      for (int n = ifs.gcount(), j = 0; j < n; ++j) {
        if (byte_index % 10 == 0) {
          if (byte_index > 0) std::cout << '\n';
          std::cout << std::setfill('0') << std::setw(9) << (byte_index * 5) << ":";
        }
        ++byte_index;

        uint8_t byte = buffer[j];
        std::cout << ' ';
        REP(_, 5) {
          int i = byte % 3;
          byte /= 3;
          ++counts[i];
          Outcome o = static_cast<Outcome>(i);
          std::cout << (o == WIN ? 'W' : o == LOSS ? 'L' : o == TIE ? 'T' : '?');
        }
      }
      std::cout << std::endl;
    }
    assert(ifs.eof() && !ifs.bad());
  }
  int64_t total = counts[0] + counts[1] + counts[2];
  std::cout << std::endl;
  std::cout << "Ties:   " << counts[TIE] << std::endl;
  std::cout << "Losses: " << counts[LOSS] << std::endl;
  std::cout << "Wins:   " << counts[WIN] << std::endl;
  std::cout << "Total:  " << total << std::endl;
}
