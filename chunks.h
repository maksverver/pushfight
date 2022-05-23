#ifndef CHUNKS_H_INCLUDED
#define CHUNKS_H_INCLUDED

#include <cstdint>
#include <string>

#include "board.h"

constexpr int chunk_size = 54054000;
constexpr int num_chunks = 7429;

static_assert(int64_t{chunk_size} * int64_t{num_chunks} == total_perms);

// Number of parts to split each chunk into. Each part is processed by 1 thread.
// A large number of parts ensures that all CPU cores are kept busy most of the
// time, but too many parts incur a lot of overhead.
constexpr int num_parts = 225;
constexpr int part_size = 240240;

static_assert(part_size * num_parts  == chunk_size);
static_assert(part_size % 16 == 0);

std::string ChunkR0FileName(const std::string &dir, int chunk);

std::string ChunkR1FileName(const std::string &dir, int chunk);

void PrintChunkUpdate(int chunk, int part);

void ClearChunkUpdate();

struct ChunkInfo {
  int phase;  // 0 or 1, or -1 for invalid
  int chunk;  // between 0 and num_chunks (exclusive), or -1 if invalid
};

ChunkInfo GetChunkInfo(const char *filename);

#endif  // ndef CHUNKS_H_INCLUDED
