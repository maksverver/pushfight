#ifndef ACCESSORS_H_INCLUDED
#define ACCESSORS_H_INCLUDED

#include <cstdlib>
#include <memory>
#include <vector>

#include "chunks.h"

static_assert(sizeof(size_t) >= 8, "Need a 64-bit OS to map large files");

void *MemMap(const char *filename, size_t length);

void MemUnmap(void *data, size_t length);

template<class T, size_t size>
class MappedFile {
public:
  static const size_t filesize = sizeof(T) * size;

  explicit MappedFile(const char *filename)
    : data_(reinterpret_cast<T*>(MemMap(filename, filesize))) {};

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
// This data stores 1 bit per permutation,  encoded with 8 bits per byte.
// 0 means TIE, 1 means WIN.
class R0Accessor {
public:
  explicit R0Accessor();

  bool operator[](size_t i) const {
    return (map[i / 8] >> (i % 8)) & 1;
  }

private:
  MappedFile<uint8_t, total_perms/8> map;
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
class R1Accessor {
public:
  explicit R1Accessor();

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
  MappedFile<uint8_t, total_perms/5> map;
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
