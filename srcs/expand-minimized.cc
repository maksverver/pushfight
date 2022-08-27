// Converts minimized.bin (~86 GB) to merged.bin (~410 GB).
//
// For reachable positions, this copies the relevant bytes from minimized.bin,
// and for unreachable positions (which are not represented in minimized.bin),
// it uses the logic from lookup-min to reconstruct the value.
//
// The merged output is written to standard output.
//
// Note that it's better to use the decompressed minimized.bin or progress will
// be very slow. Even then, this tool is pretty slow. I ran it on a Google Cloud
// Engine C2 instance (30 vCPU, 120 GB memory) and it took around 4 minutes per
// chunk, which implies 20 days to reconstruct the file fully.

#include "board.h"
#include "chunks.h"
#include "macros.h"
#include "minimized-accessor.h"
#include "minimized-lookup.h"
#include "parse-int.h"
#include "perms.h"
#include "position-value.h"

#include <iostream>
#include <optional>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
// By default, use 1.5 times the number of cores.
const int thread_count = (std::thread::hardware_concurrency() * 3 + 1) / 2;

std::optional<MinimizedAccessor> acc;

void ExpandThread(int chunk, std::atomic<int> *next_part, uint8_t bytes[]) {
  const int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  for (;;) {
    const int part = (*next_part)++;
    if (part + 1 >= thread_count) PrintChunkUpdate(chunk, part + 1 - thread_count);
    if (part >= num_parts) break;  // note: will actually exceed num_parts!
    int part_start = part * part_size;
    Perm perm = PermAtIndex(start_index + part_start);
    REP(i, part_size) {
      bytes[part_start + i] = LookupValue(*acc, perm, nullptr).value().byte;
      std::next_permutation(perm.begin(), perm.end());
    }
  }
}

std::vector<uint8_t> Expand(int chunk) {
  std::vector<uint8_t> bytes(chunk_size);
  std::atomic<int> next_part = 0;
  if (thread_count == 0) {
    // Single-threaded computation.
    ExpandThread(chunk, &next_part, bytes.data());
  } else {
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    REP(i, thread_count) {
      threads.emplace_back(ExpandThread, chunk, &next_part, bytes.data());
    }
    REP(i, thread_count) threads[i].join();
    assert(next_part == num_parts + thread_count);
  }
  ClearChunkUpdate();
  return bytes;
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) {
    std::cerr
        << "Usage: expand-minimized <minimized.bin> <start-chunk> <end-chunk>\n\n"
        << "e.g. `expand-minimized minimized.bin 0 7429 > merged.bin` to expand everything"
        << std::endl;
    return 1;
  }

  int start_chunk = std::max(ParseInt(argv[2]), 0);
  int end_chunk = std::min(ParseInt(argv[3]), num_chunks);

  acc.emplace(argv[1]);

  FOR(chunk, start_chunk, end_chunk) {
    std::vector<uint8_t> bytes = Expand(chunk);
    assert(bytes.size() == chunk_size);
    std::cout.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
  }
}
