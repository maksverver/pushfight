#ifndef ACCESSORS_H_INCLUDED
#define ACCESSORS_H_INCLUDED

#include <cstdlib>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "chunks.h"

static_assert(sizeof(size_t) >= 8, "Need a 64-bit OS to map large files");

void *MemMap(const char *filename, size_t length, bool writable);

void MemUnmap(void *data, size_t length);

template<class T, size_t size>
class MappedFile {
public:
  enum Access { READONLY, READWRITE };

  static const size_t filesize = sizeof(T) * size;

  explicit MappedFile(const char *filename, Access access = READONLY)
    : data_(reinterpret_cast<T*>(MemMap(filename, filesize, access == READWRITE))) {};

  const T* data() const { return data_.get(); }
  const T& operator[](size_t i) const { return data()[i]; }

private:
  struct Unmapper {
    void operator()(T *data) {
      MemUnmap(data, filesize);
    }
  };

  std::unique_ptr<T, Unmapper> data_;
};

// Accessor for result data of phase 0 (written by solve-r0) merged into a
// single file.
//
// This data stores 1 bit per permutation, encoded with 8 bits per byte.
// 0 means TIE, 1 means WIN.
template<size_t filesize>
class R0AccessorBase {
public:
  explicit R0AccessorBase(const char *filename) : map(filename) {}

  bool operator[](size_t i) const {
    return (map[i / 8] >> (i % 8)) & 1;
  }

private:
  MappedFile<uint8_t, filesize> map;
};

class R0Accessor : public R0AccessorBase<total_perms/8> {
public:
  explicit R0Accessor() : R0AccessorBase("input/r0.bin") {}
};

class R0ChunkAccessor : public R0AccessorBase<chunk_size/8> {
public:
  explicit R0ChunkAccessor(const char *filename) : R0AccessorBase(filename) {}
};

// Accessor for result data of phase 0 (written by solve-r0) as separate
// chunk files.
//
// This data consists of 7429 chunks, each storing 1 bit per permutation,
// encoded with 8 bits per byte. 0 means TIE, 1 means WIN.
class ChunkedR0Accessor {
public:
  explicit ChunkedR0Accessor();

  bool operator[](size_t i) const {
    size_t chunk = i / chunk_size;
    size_t index = i % chunk_size;
    return (maps[chunk][index / 8] >> (index % 8)) & 1;
  }

private:
  std::vector<MappedFile<uint8_t, chunk_size/8>> maps;
};

// Accessor for result data of phase 1 (written by solve-r1) merged into a
// single file.
//
// This data stores an Outcome value per permutation (0 for TIE, 1 for LOSS,
// 2 for WIN). The results are encoded in ternary with 5 values per byte (or
// 1.6 bits per value).
template<size_t filesize>
class RnAccessorBase {
public:
  explicit RnAccessorBase(const char *filename) : map(filename) {}

  Outcome operator[](size_t i) const {
    uint8_t byte = map[i / 5];
    switch (i % 5) {
    case 0: return static_cast<Outcome>(byte % 3);
    case 1: return static_cast<Outcome>(byte / 3 % 3);
    case 2: return static_cast<Outcome>(byte / (3 * 3) % 3);
    case 3: return static_cast<Outcome>(byte / (3 * 3 * 3) % 3);
    case 4: return static_cast<Outcome>(byte / (3 * 3 * 3 * 3) % 3);
    default: abort();  // unreachable
    }
  }

private:
  MappedFile<uint8_t, filesize> map;
};

class RnAccessor : public RnAccessorBase<total_perms/5> {
public:
  explicit RnAccessor(const char *filename) : RnAccessorBase(filename) {}
};

class RnChunkAccessor : public RnAccessorBase<chunk_size/5> {
public:
  explicit RnChunkAccessor(const char *filename) : RnAccessorBase(filename) {}
};

// Accessor for result data of phase 1 (written by solve-r1) as separate
// chunk files.
//
// This data consists of 7429 chunks, each storing an Outcome value
// per permutation (0 for TIE, 1 for LOSS, 2 for WIN). The results are
// encoded in ternary with 5 values per byte (or 1.6 bits per value).
class ChunkedR1Accessor {
public:
  explicit ChunkedR1Accessor();

  Outcome operator[](size_t i) const {
    size_t chunk = i / chunk_size;
    size_t index = i % chunk_size;
    uint8_t byte = maps[chunk][index / 5];
    switch (index % 5) {
    case 0: return static_cast<Outcome>(byte % 3);
    case 1: return static_cast<Outcome>(byte / 3 % 3);
    case 2: return static_cast<Outcome>(byte / (3 * 3) % 3);
    case 3: return static_cast<Outcome>(byte / (3 * 3 * 3) % 3);
    case 4: return static_cast<Outcome>(byte / (3 * 3 * 3 * 3) % 3);
    default: abort();  // unreachable
    }
  }

private:
  std::vector<MappedFile<uint8_t, chunk_size/5>> maps;
};

#endif  // ndef ACCESSORS_H_INCLUDED
