#include "chunks.h"

#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

std::string ChunkFileName(int phase, const std::string &dir, int chunk) {
  std::ostringstream oss;
  if (!dir.empty()) oss << dir << '/';
  oss << "chunk-r" << phase << "-" << std::setfill('0') << std::setw(4) << chunk << ".bin";
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
  std::cerr << "\n";
}

ChunkInfo GetChunkInfo(const char *filename) {
  for (const char *p = filename; *p; ++p) if (*p == '/') filename = p + 1;

  ChunkInfo result = { -1, -1 };
  if (strncmp(filename, "chunk-r0-", 9) == 0) {
    result.phase = 0;
    filename += 9;
  } else if (strncmp(filename, "chunk-r1-", 9) == 0) {
    result.phase = 1;
    filename += 9;
  } else {
    return result;
  }
  std::istringstream iss(filename);
  int i = -1;
  if ((iss >> i) && (i >= 0 && i < num_chunks)) {
    result.chunk = i;
  }
  return result;
}
