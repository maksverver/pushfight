// Tool to verify rN.bin files using sha256 checksums of some chunks.
//
// This is basically a thin wrapper around the VerifyInputChunks() function.

#include "accessors.h"
#include "chunks.h"
#include "parse-int.h"
#include "input-verification.h"

#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4) {
    std::cout << "Usage: verify-input-chunks <rN.bin> <phase> [<count>]" << std::endl;
    return 1;
  }
  int phase = ParseInt(argv[2]);
  if (phase < 1) {
    std::cout << "Invalid phase! Must be at least 1." << std::endl;
    return 1;
  }
  int chunks_to_verify = argc > 3 ? ParseInt(argv[3]) : num_chunks;
  if (chunks_to_verify < 1 || chunks_to_verify > num_chunks) {
    std::cout << "Invalid number of chunks to verify!" << std::endl;
    return 1;
  }
  RnAccessor acc(argv[1]);
  int failures = VerifyInputChunks(phase, acc, chunks_to_verify);
  if (failures > 0) {
    std::cerr << failures << " total failures!" << std::endl;
    return failures;
  }

  std::cerr << "Succesfully verified ";
  if (chunks_to_verify == num_chunks) {
    std::cerr << "all ";
  } else {
    std::cerr << chunks_to_verify << " of " << num_chunks;
  }
  std::cerr  << " chunks!" << std::endl;
}
