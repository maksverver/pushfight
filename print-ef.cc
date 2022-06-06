// Decodes Elias-Fano encoded output files and print the numbers one per line.
//
// Note this doesn't stream; it reads the whole input file into memory, so it
// doesn't work well with very large files.

#include "bytes.h"
#include "efcodec.h"
#include "parse-int.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: print-ef [<--part=N>] <file...>\n\nUse \"-\" to read from stdin." << std::endl;
    return 1;
  }
  int start_argi = 1;
  int want_part = -1;
  if (strncmp(argv[start_argi], "--part=", 7) == 0) {
    want_part = ParseInt(argv[start_argi] + 7);
    ++start_argi;
  }

  for (int i = start_argi; i < argc; ++i) {
    bytes_t bytes;
    if (strcmp(argv[i], "-") == 0) {
      bytes = ReadInput(std::cin);
    } else {
      std::ifstream ifs;
      ifs.rdbuf()->pubsetbuf(nullptr, 0);
      ifs.open(argv[i], std::ifstream::binary);
      if (!ifs) {
        std::cerr << "Failed to open input: " << argv[i] << std::endl;
        return 1;
      }
      bytes = ReadInput(ifs);
    }
    byte_span_t byte_span = bytes;
    for (int part = 0; !byte_span.empty(); ++part) {
      auto result = DecodeEF(&byte_span);
      if (!result) {
        std::cerr << "Elias-Fano decoding failed!" << std::endl;
        return 1;
      }
      if (want_part == -1 || part == want_part) {
        for (auto i : *result) std::cout << i << '\n';
      }
    }
  }
  std::cout << std::flush;
}
