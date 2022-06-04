#include "bytes.h"

#include <cassert>
#include <filesystem>
#include <fstream>

#include "byte_span.h"

bytes_t ReadFromFile(const std::string &filename) {
  size_t size = std::filesystem::file_size(filename);
  bytes_t bytes(size);
  std::ifstream ifs(filename, std::ofstream::binary);
  if (!ifs) {
    std::cerr << "Failed to open input file: " << filename << std::endl;
    exit(1);
  }
  if (!ifs.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
    std::cerr << "Failed to read " << size << " bytes from file: " << filename << std::endl;
    exit(1);
  }
  assert(ifs.gcount() == bytes.size());
  return bytes;
}

void WriteToFile(const std::string &filename, byte_span_t bytes) {
  std::ofstream ofs(filename, std::ofstream::binary);
  if (!ofs) {
    std::cerr << "Failed to open output file: " << filename << std::endl;
    exit(1);
  }
  if (!ofs.write(reinterpret_cast<const char*>(bytes.data()), bytes.size())) {
    std::cerr << "Failed to write " << bytes.size() << " bytes to file: " << filename << std::endl;
    exit(1);
  }
}
