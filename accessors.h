#ifndef ACCESSORS_H_INCLUDED
#define ACCESSORS_H_INCLUDED

#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "chunks.h"
#include "ternary.h"

static_assert(sizeof(size_t) >= 8, "Need a 64-bit OS to map large files");

void *MemMap(const char *filename, size_t length, bool writable);

void MemUnmap(void *data, size_t length);

template<class T, size_t size>
class MappedFile {
public:
  static const size_t filesize = sizeof(T) * size;

  explicit MappedFile(const char *filename) : MappedFile(filename, false) {}

  const T* data() const { return data_.get(); }
  const T& operator[](size_t i) const { return data()[i]; }

protected:
  MappedFile(const char *filename, bool writable)
    : data_(reinterpret_cast<T*>(MemMap(filename, filesize, writable))) {};

  struct Unmapper {
    void operator()(T *data) {
      MemUnmap(data, filesize);
    }
  };

  std::unique_ptr<T, Unmapper> data_;
};

template<class T, size_t size>
class MutableMappedFile : public MappedFile<T, size> {
public:
  MutableMappedFile(const char *filename) : MappedFile<T, size>(filename, true) {}

  T* data() { return MappedFile<T, size>::data_.get(); }
  T& operator[](size_t i) { return data()[i]; }

  const T* data() const { return MappedFile<T, size>::data_.get(); }
  const T& operator[](size_t i) const { return data()[i]; }
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
    return static_cast<Outcome>(DecodeTernary(map[i / 5], i));
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


template<class A, class T>
class AccessorReference {
public:
  AccessorReference(A *acc, size_t i) : acc(acc), i(i) {}

  operator Outcome() const {
    return acc->get(i);
  }

  AccessorReference<A, T>& operator=(T v) {
    acc->set(i, v);
    return *this;
  }

private:
  A *acc;
  size_t i;
};

class MutableRnAccessor {
public:
  using Reference = AccessorReference<MutableRnAccessor, Outcome>;

  explicit MutableRnAccessor(const char *filename) : map(filename) {}

  Outcome get(size_t i) const {
    return static_cast<Outcome>(DecodeTernary(map[i / 5], i));
  }

  void set(size_t i, Outcome o) {
    uint8_t &byte = map[i / 5];
    byte = EncodeTernary(byte, i, static_cast<int>(o));
  }

  Outcome operator[](size_t i) const {
    return get(i);
  }

  Reference operator[](size_t i) {
    return Reference(this, i);
  }

private:
  MutableMappedFile<uint8_t, chunk_size/5> map;
};

// Accessor wrapper that wraps get() and set() calls with a mutex guard.
template<class T, class A>
class ThreadSafeAccessor {
public:
  using Reference = AccessorReference<ThreadSafeAccessor<T, A>, T>;

  ThreadSafeAccessor(const char *filename) : delegate_accessor(filename) {}

  T get(size_t i) const {
    std::lock_guard<std::mutex> guard(mutex);
    return delegate_accessor.get(i);
  }

  void set(size_t i, T v) {
    std::lock_guard<std::mutex> guard(mutex);
    delegate_accessor.set(i, v);
  }

  T operator[](size_t i) const {
    return get(i);
  }

  Reference operator[](size_t i) {
    return Reference(this, i);
  }

private:
  mutable std::mutex mutex;
  A delegate_accessor;
};

using ThreadSafeMutableRnAccessor = ThreadSafeAccessor<Outcome, MutableRnAccessor>;

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

// Mutable accessor for binary data stored in a single file.
//
// Similar to R0AccessorBase except this also supports mutations.
template<size_t filesize>
class MutableBinaryAccessor {
public:
  using Reference = AccessorReference<MutableBinaryAccessor, bool>;

  explicit MutableBinaryAccessor(const char *filename) : map(filename) {}

  bool get(size_t i) const {
    return (map[i / 8] >> (i % 8)) & 1;
  }

  void set(size_t i, bool v) {
    uint8_t &byte = map[i / 8];
    uint8_t new_byte = (byte & ~(uint8_t{1} << (i % 8))) | (uint8_t{v} << (i % 8));
    // Only assigning when the byte changed might be more efficient for memory
    // mapped files, since fewer pages end up being marked dirty?
    if (byte != new_byte) byte = new_byte;
  }

  Outcome operator[](size_t i) const {
    return get(i);
  }

  Reference operator[](size_t i) {
    return Reference(this, i);
  }

private:
  MutableMappedFile<uint8_t, filesize> map;
};

template<size_t filesize>
using ThreadSafeMutableBinaryAccessor = ThreadSafeAccessor<bool, MutableBinaryAccessor<filesize>>;

// Accessor used to generate output files in backprogate-losses.cc.
//
// For each permutation, it stores a single bit indicating whether this
// permutation is newly won. Additionaly, it stores a bitmask of
// previously-processed chunks. This allows output files to be merged.
class LossPropagationAccessor {
public:
  LossPropagationAccessor(const char *filename) : acc(CheckOutputFile(filename)) {}

  void MarkWinning(int64_t index) {
    acc[winning_offset + index] = true;
  }

  bool IsChunkComplete(int chunk) const {
    return acc[chunk];
  }

  void MarkChunkComplete(int chunk) {
    acc[chunk] = true;
  }

private:
  const char *CheckOutputFile(const char *filename);

  static constexpr size_t winning_offset = 4096;
  static constexpr size_t filesize = total_perms/8 + (num_chunks + 7)/8;

  static_assert(winning_offset * 8 >= num_chunks);

  ThreadSafeMutableBinaryAccessor<filesize> acc;
};

#endif  // ndef ACCESSORS_H_INCLUDED
