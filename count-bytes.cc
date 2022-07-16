// Simple too to count the frequency of bytes in a file.
//
// Prints lines of pairs: byte, frequence, separated by a tab.
//
// Output is sorted by byte values. If you want something else, sort with
// e.g. `sort -n -k 2 -r` to sort by frequency, descending.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
  int64_t freq[256] = {};
  for (int i = 1; i < argc; ++i) {
    std::ifstream ifs(argv[i], std::ifstream::binary);
    while (ifs) {
      uint8_t buffer[409600];
      ifs.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
      for (int n = ifs.gcount(), j = 0; j < n; ++j) {
        ++freq[buffer[j]];
      }
    }
    assert(ifs.eof() && !ifs.bad());
  }
  int max_freq = 255;
  while (max_freq > 0 && freq[max_freq] == 0) --max_freq;
  for (int i = 0; i <= max_freq; ++i) {
    std::cout << i << '\t' << freq[i] << std::endl;
  }
}
