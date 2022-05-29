// Encodes the difference between two ternary files into binary.
//
// The sizes of the files must be equal, and the number of values must be
// a multiple of 8, which means the file sizes must be a multiple of 8
// (since each input file packs 5 ternary values in 1 byte).
//
// This tool can be used to generate the delta between two phase outputs,
// which improves compression, and can also be used with backpropagate-losses
// for later phases.
//
// Note that during each phase, only ties change to either win or loss.
// These changes can therefore be encoded in binary, e.g.:
//
//  phase 1: TTTWWWTTTLLLTTT
//  phase 2: TTWWWWWTTLLLTTW
//  delta:   001000100000001
//
// This tool autodetects which change is being encoded (T->W or T->L),
// but note that it must be specified explicitly when decoding.

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
  if (argc != 3) {
    std::cerr << "Usage: encode-delta <r(N-1).bin> <rN.bin>\n\n"
      << "Note: writes delta bitmap to standard output!" << std::endl;
    return 0;
  }

  std::ifstream ifs1 = OpenInputUnbuffered(argv[1]);
  if (!ifs1) {
    std::cerr << "Failed to open file 1 ( " << argv[1] << ")!" << std::endl;
    return 1;
  }
  std::ifstream ifs2 = OpenInputUnbuffered(argv[2]);
  if (!ifs2) {
    std::cerr << "Failed to open file 2 (" << argv[2] << ")!" << std::endl;
    return 1;
  }

  TernaryReader r1(ifs1);
  TernaryReader r2(ifs2);

  // Try to make writing to std::cout less slow.
  std::ios::sync_with_stdio(false);
  std::cout.rdbuf()->pubsetbuf(nullptr, 0);  // Not sure if this works...
  BinaryWriter w(std::cout);

  // Direction of changes: TIE->WIN or TIE->LOSS. We will autodetect this later,
  // so initialize to TIE to signal we haven't decided yet.
  Outcome direction = TIE;
  int64_t i, ones = 0;
  for (i = 0; r1.HasNext() && r2.HasNext(); ++i) {
    Outcome o1 = r1.Next();
    Outcome o2 = r2.Next();
    bool delta = o1 != o2;
    w.Write(delta);
    if (delta) {
      // Decide direction
      if (direction == TIE) {
        direction = o2;
        std::cerr << "Detected direction: " << OutcomeToString(direction) << std:: endl;
      }

      if (o1 != TIE || o2 != direction) {
        std::cerr << "Invalid transition at index " << i << ": "
            << OutcomeToString(o1) << " -> " << OutcomeToString(o2) << std::endl;
        return 1;
      }

      ++ones;
    }

    if ((i + 1) % 1000000000 == 0) {
      std::cerr << (i + 1) / 1000000000 << " billion bits written..." << std::endl;
    }
  }
  std::cerr << "Output direction was: " << OutcomeToString(direction) << std::endl;
  std::cerr << "Output has " << ones << " ones out of " << i << " total bits." << std::endl;

  if (r1.HasNext()) {
    std::cerr << "File 1 is longer than file 2!" << std::endl;
    return 1;
  }
  if (r2.HasNext()) {
    std::cerr << "File 2 is longer than file 1!" << std::endl;
    return 1;
  }
}
