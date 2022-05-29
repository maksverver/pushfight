// Generates a ternary file from a base ternary file and a binary delta file.
//
// The number of values in the files must match. That means the ternary file
// must be exactly 1.6 times as large as the binary file.
//
// This tool is the inverse of encode-delta.cc.

#include <cstring>
#include <iostream>
#include <fstream>

#include "codec.h"
#include "macros.h"

namespace {

std::ifstream OpenInputUnbuffered(const char *filename) {
  std::ifstream ifs;
  ifs.rdbuf()->pubsetbuf(nullptr, 0);
  ifs.open(filename, std::ifstream::binary);
  return ifs;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: decode-delta <direction> <r(N-1).bin> <delta.bin>\n\n"
      << "Direction is one of WIN or LOSS. It's VERY IMPORTANT that this matches the\n"
      << "direction detected when generating the delta, or the result will be wrong!\n\n"
      << "Note: writes outcomes to standard output!" << std::endl;
    return 0;
  }

  Outcome direction = TIE;
  if (strcmp(argv[1], "WIN") == 0) {
    direction = WIN;
  } else if (strcmp(argv[1], "LOSS") == 0) {
    direction = LOSS;
  } else {
    std::cerr << "Invalid direction: " << argv[1] << " (must be WIN or LOSS)." << std::endl;
    return 1;
  }

  std::ifstream ifs1 = OpenInputUnbuffered(argv[2]);
  if (!ifs1) {
    std::cerr << "Failed to open file 1 ( " << argv[2] << ")!" << std::endl;
    return 1;
  }
  std::ifstream ifs2 = OpenInputUnbuffered(argv[3]);
  if (!ifs2) {
    std::cerr << "Failed to open file 2 (" << argv[3] << ")!" << std::endl;
    return 1;
  }

  TernaryReader r1(ifs1);
  BinaryReader r2(ifs2);

  // Try to make writing to std::cout less slow.
  std::ios::sync_with_stdio(false);
  std::cout.rdbuf()->pubsetbuf(nullptr, 0);  // Not sure if this works...
  TernaryWriter w(std::cout);

  int64_t count[3] = {0, 0, 0};
  int64_t i, changed = 0;
  for (i = 0; r1.HasNext() && r2.HasNext(); ++i) {
    Outcome o = r1.Next();
    bool delta = r2.Next();
    if (delta) {
      if (o != TIE) {
        std::cerr << "Invalid transition at index " << i << ": "
            << OutcomeToString(o) << " -> " << OutcomeToString(direction) << std::endl;
        return 1;
      }
      o = direction;
      ++changed;
    }
    w.Write(o);
    ++count[o];

    if ((i + 1) % 1000000000 == 0) {
      std::cerr << (i + 1) / 1000000000 << " billion values written..." << std::endl;
    }
  }
  std::cerr << changed << " values changed from TIE to " << OutcomeToString(direction) << std::endl;
  std::cerr << i << " values written (" << count[TIE] << " ties, " << count[LOSS] << " losses, " << count[WIN] << " wins)" << std::endl;

  if (r1.HasNext()) {
    std::cerr << "File 1 is longer than file 2!" << std::endl;
    return 1;
  }
  if (r2.HasNext()) {
    std::cerr << "File 2 is longer than file 1!" << std::endl;
    return 1;
  }
}
