#include "chunks.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

std::string ChunkR0FileName(int chunk) {
  std::ostringstream oss;
  oss << "results/chunk-r0-" << std::setfill('0') << std::setw(4) << chunk << ".bin";
  return oss.str();
}

std::string ChunkR1FileName(int chunk) {
  std::ostringstream oss;
  oss << "results/chunk-r1-" << std::setfill('0') << std::setw(4) << chunk << ".bin";
  return oss.str();
}

void PrintChunkUpdate(int chunk, int part) {
  // Precompute output line to minimize formatting errors due to multiple
  // threads writing to std::cerr at the same time.
  char buf[100];
  snprintf(buf, sizeof(buf),
      "Chunk %d calculating... part %d / %d (%.2f%% done)\r",
      chunk, part, num_parts, 100.0*part / num_parts);
  std::cerr << buf << std::flush;
}

void ClearChunkUpdate() {
  std::cerr << "                                                            \r";
}
