// Tool to convert the merged phase output (401,567,166,000 bytes) to
// minimized output (86,208,131,520 bytes).
//
// In the minimized output, only results for reachable states are stored
// using minified permutation indices.

#include "accessors.h"
#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "perms.h"
#include "search.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

bool CreateNewFile(const char *filename, std::uintmax_t filesize) {
  if (std::filesystem::exists(filename)) {
    std::cerr << "File already exists: " << filename << std::endl;
    return false;
  }
  std::cerr << "Creating new file " << filename << "..."
      << " (" << min_index_size / 1e9 << " GB)" << std::endl;
  std::ofstream ofs(filename, std::ofstream::binary);
  if (!ofs) {
    std::cerr << "Failed to create file!" << std::endl;
    return false;
  }
  ofs.close();
  std::filesystem::resize_file(filename, filesize);
  assert(std::filesystem::file_size(filename) == filesize);
  return true;
}

void HandlePerm(
    const Perm &perm, uint8_t value,
    MutableMappedFile<uint8_t, min_index_size> &output) {
  if (!IsReachable(perm)) return;

  bool rotated = false;
  int64_t min_index = MinIndexOf(perm, &rotated);
  assert(0 <= min_index && min_index < min_index_size);
  if (rotated) return;

  assert(output[min_index] == 0);
  if (value != 0) {
    output[min_index] = value;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  InitializePerms();

  if (argc != 2) {
    std::cerr << "Usage: minify-merged <minimized.bin>\n"
        "Merged output is read from standard input." << std::endl;
    return 1;
  }

  const char *output_filename = argv[1];

  // Create and open output file.
  if (!CreateNewFile(output_filename, min_index_size)) {
    return 1;
  }
  MutableMappedFile<uint8_t, min_index_size> output(output_filename);

  // Iterate over all permutations.
  Perm perm = first_perm;
  int64_t index = 0;
  REP(chunk, num_chunks) {
    assert(index == int64_t{chunk} * chunk_size);
    assert(perm == PermAtIndex(index));

    std::vector<uint8_t> buffer(chunk_size);
    if (!std::cin.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) ||
        std::cin.gcount() != chunk_size) {
      std::cerr << "Failed to read input chunk " << chunk << "!" << std::endl;
      return 1;
    }

    for (uint8_t byte : buffer) {
      HandlePerm(perm, byte, output);
      ++index;
      std::next_permutation(perm.begin(), perm.end());
    }

    std::cerr << "Chunk " << chunk << " / " << num_chunks << " done." << std::endl;
  }
  assert(index == total_perms);
  return 0;
}
