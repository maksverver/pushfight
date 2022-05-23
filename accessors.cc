#include "accessors.h"

#include "macros.h"

#include <filesystem>
#include <iostream>

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

#if _WIN32

#include <windows.h>
#include <memoryapi.h>

static_assert(sizeof(SIZE_T) >= 8);

void *MemMapWindows(const char *filename, size_t length) {
  HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == NULL) {
    std::cerr << "CreateFile() failed on file " << filename
        << " error code " << GetLastError() << std::endl;
    exit(1);
  }
  HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
  CloseHandle(hFile);
  if (hMapping == NULL) {
    std::cerr << "CreateFileMapping() failed on file " << filename
        << " error code " << GetLastError() << std::endl;
    exit(1);
  }
  LPVOID data = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, SIZE_T{length});
  CloseHandle(hMapping);
  if (data == nullptr) {
    std::cerr << "MapViewOfFile() failed on file " << filename
        << " error code " << GetLastError() << std::endl;
    exit(1);
  }
  return data;
}

void MemUnmapWindows(void *data, size_t length) {
  (void) length;  // unused
  if (!UnmapViewOfFile(data)) {
    std::cerr << "UnmapViewOfFile() failed" << std::endl;
    // Don't abort.
  }
}

#else

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void *MemMapLinux(const char *filename, size_t length) {
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    std::cerr << "Failed to open() " << filename << std::endl;
    exit(1);
  }
  // Apparently MAP_SHARED works on Windows Subsystem for Linux
  // while MAP_PRIVATE does not?
  void *data = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  if (data == MAP_FAILED) {
    std::cerr << "Failed to mmap() " << filename << std::endl;
    exit(1);
  }
  return data;
}

void MemUnmapLinux(void *data, size_t length) {
  if (munmap(data, length) != 0) {
    std::cerr << "munmap() failed" << std::endl;
    // Don't abort.
  }
}

#endif // def __linux__

}  // namespace

void *MemMap(const char *filename, size_t length) {
  CheckFileSize(filename, length);

#if _WIN32
  return MemMapWindows(filename, length);
#else
  return MemMapLinux(filename, length);
#endif
}

void MemUnmap(void *data, size_t length) {
#if _WIN32
  return MemUnmapWindows(data, length);
#else
  return MemUnmapLinux(data, length);
#endif
}

ChunkedR0Accessor::ChunkedR0Accessor() {
  maps.reserve(num_chunks);
  REP(chunk, num_chunks) {
    maps.emplace_back(ChunkFileName(0, "input", chunk).c_str());
  }
}

ChunkedR1Accessor::ChunkedR1Accessor() {
  maps.reserve(num_chunks);
  REP(chunk, num_chunks) {
    maps.emplace_back(ChunkFileName(1, "input", chunk).c_str());
  }
}
