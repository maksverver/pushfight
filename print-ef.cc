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
    std::cerr <<
        "Usage: print-ef [<--part=N>] [<--every=M>] <file...>\n\n"
        "With --part=N, prints only the N-th part (0-based).\n"
        "With --part=N --every=M, prints only the parts with index N modulo M\n"
        "(e.g. --part=1 --every=2 prints all the odd-numbered parts).\n\n"
        "Use \"-\" to read from stdin." << std::endl;
    return 1;
  }
  int start_argi = 1;
  int want_part = -1;
  if (strncmp(argv[start_argi], "--part=", 7) == 0) {
    want_part = ParseInt(argv[start_argi] + 7);
    ++start_argi;
  }
  int every = -1;
  if (strncmp(argv[start_argi], "--every=", 8) == 0) {
    if (want_part < 0) {
      std::cerr << "Cannot use --every=M withouth preceding --part=N." << std::endl;
      return 1;
    }
    every = ParseInt(argv[start_argi] + 8);
    if (every <= want_part) {
      std::cerr << "Argument to --part must be strictly less than argument to --every." << std::endl;
      return 1;
    }
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
      if (every > 0 ? part % every == want_part :
          (want_part == -1 || part == want_part)) {
        for (auto i : *result) std::cout << i << '\n';
      }
    }
  }
  std::cout << std::flush;
}
