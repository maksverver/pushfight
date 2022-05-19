#include "accessors.h"

#include "macros.h"

#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

void *MemMap(const char *filename, size_t length) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open() " << filename << std::endl;
    exit(1);
  }
  void *data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (!data) {
    std::cerr << "Failed to mmap() " << filename << std::endl;
    exit(1);
  }
  return data;
}

void MemUnmap(void *data, size_t length) {
  if (munmap(data, length) != 0) {
    std::cerr << "munmap() failed" << std::endl;
    // Don't abort.
  }
}

R0Accessor::R0Accessor() {
  maps.reserve(num_chunks);
  REP(chunk, num_chunks) {
    maps.emplace_back(ChunkR0FileName(chunk).c_str());
  }
}

R1Accessor::R1Accessor() {
  maps.reserve(num_chunks);
  // TODO: uncomment the real implementation
  // REP(chunk, num_chunks) {
  FOR(chunk, 7300, 7304) {
    maps.emplace_back(ChunkR1FileName(chunk).c_str());
  }
}
