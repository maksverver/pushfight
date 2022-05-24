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

void *MemMapWindows(const char *filename, size_t length, bool writable) {
  HANDLE hFile = NULL;
  {
    DWORD dwDesiredAccess = writable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    DWORD dwShareMode = writable ? 0 : FILE_SHARE_READ;
    DWORD dwCreationDisposition = OPEN_EXISTING;
    DWORD dwFlagsAndAttributes = writable ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_READONLY;
    hFile = CreateFile(
        filename, dwDesiredAccess, dwShareMode, NULL,
        dwCreationDisposition, dwFlagsAndAttributes, NULL);
    if (hFile == NULL) {
      std::cerr << "CreateFile() failed on file " << filename
          << " error code " << GetLastError() << std::endl;
      exit(1);
    }
  }
  HANDLE hMapping = NULL;
  {
    DWORD protect = writable ? PAGE_READWRITE : PAGE_READONLY;
    hMapping = CreateFileMapping(hFile, NULL, protect, 0, 0, NULL);
    CloseHandle(hFile);
    if (hMapping == NULL) {
      std::cerr << "CreateFileMapping() failed on file " << filename
          << " error code " << GetLastError() << std::endl;
      exit(1);
    }
  }
  LPVOID data = NULL;
  {
    DWORD dwDesiredAccess = writable ? (FILE_MAP_WRITE | FILE_MAP_READ) : FILE_MAP_READ;
    data = MapViewOfFile(hMapping, dwDesiredAccess, 0, 0, SIZE_T{length});
    CloseHandle(hMapping);
    if (data == NULL) {
      std::cerr << "MapViewOfFile() failed on file " << filename
          << " error code " << GetLastError() << std::endl;
      exit(1);
    }
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

void *MemMapLinux(const char *filename, size_t length, bool writable) {
  int mode = writable ? O_RDWR : O_RDONLY;
  int fd = open(filename, mode);
  if (fd == -1) {
    std::cerr << "Failed to open() " << filename << std::endl;
    exit(1);
  }
  // Apparently MAP_SHARED works on Windows Subsystem for Linux
  // while MAP_PRIVATE does not?
  int prot = writable ? (PROT_READ | PROT_WRITE) : PROT_READ;
  void *data = mmap(nullptr, length, prot, MAP_SHARED, fd, 0);
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

void *MemMap(const char *filename, size_t length, bool writable) {
  CheckFileSize(filename, length);

#if _WIN32
  return MemMapWindows(filename, length, writable);
#else
  return MemMapLinux(filename, length, writable);
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
