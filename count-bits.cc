#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
  int64_t ones = 0;
  int64_t zeros = 0;
  for (int i = 1; i < argc; ++i) {
    std::ifstream ifs(argv[i]);
    while (ifs) {
      uint8_t buffer[409600];
      ifs.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
      for (int n = ifs.gcount(), j = 0; j < n; ++j) {
        uint8_t byte = buffer[j];
        for (int bit = 0; bit < 8; ++bit) {
          if (byte & 1) ++ones; else ++zeros;
          byte >>= 1;
        }
      }
    }
    assert(ifs.eof() && !ifs.bad());
  }
  int64_t total = zeros + ones;
  std::cout << "Zero bits:  " << zeros << std::endl;
  std::cout << "One bits:   " << ones << std::endl;
  std::cout << "Total bits: " << total << std::endl;
  std::cout << "Fraction of ones: " << (total > 0 ? 1.0*ones / total : 0) << std::endl;
}
