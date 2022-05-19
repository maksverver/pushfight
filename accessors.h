#ifndef ACCESSORS_H_INCLUDED
#define ACCESSORS_H_INCLUDED

#include <memory>
#include <vector>

#include "chunks.h"

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

class R0Accessor {
public:
  explicit R0Accessor();

  bool operator[](size_t i) const {
    size_t chunk = i / chunk_size;
    size_t index = i % chunk_size;
    return (maps[chunk][index / 8] >> (index % 8))&1;
  }

private:
  std::vector<MappedFile<uint8_t, chunk_size/8>> maps;
};

#endif  // ndef ACCESSORS_H_INCLUDED
