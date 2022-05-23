#include "accessors.h"

#include "macros.h"

#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>

namespace {

void CheckFileSize(const char *filename, size_t size) {
  if (!std::filesystem::exists(filename)) {
    std::cerr << "File " << filename << " does not exist.\n";
    exit(1);
  }
  auto filesize = std::filesystem::file_size(filename);
  if (filesize < size) {
    std::cerr << "File " << filename << " is too short. "
        "Expected " << size << " bytes, actual " << filesize << " bytes.\n";
    exit(1);
  }
  if (filesize > size) {
    std::cerr << "WARNING: file " << filename << " is too long. "
        "Expected " << size << " bytes, actual " << filesize << " bytes.\n";
    // Don't exit. We should still be able to map the prefix.
  }
}

}  // namespace

void *MemMap(const char *filename, size_t length) {
  CheckFileSize(filename, length);

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open() " << filename << std::endl;
    exit(1);
  }
  void *data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  if (data == MAP_FAILED) {
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

R0Accessor::R0Accessor() : map("input/r0.bin") {}

ChunkedR0Accessor::ChunkedR0Accessor() {
  maps.reserve(num_chunks);
  REP(chunk, num_chunks) {
    maps.emplace_back(ChunkR0FileName(chunk).c_str());
  }
}

R1Accessor::R1Accessor() : map("input/r1.bin") {}

ChunkedR1Accessor::ChunkedR1Accessor() {
  maps.reserve(num_chunks);
  REP(chunk, num_chunks) {
    maps.emplace_back(ChunkR1FileName(chunk).c_str());
  }
}
