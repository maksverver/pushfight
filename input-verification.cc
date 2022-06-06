#include "input-verification.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <random>

#include "accessors.h"
#include "hash.h"

// This class has friend access to RnAccessor to be able to access the
// underlying memory mapped file.
class ChunkVerifier {
public:
  constexpr static size_t chunk_byte_size = chunk_size / 5;

  ChunkVerifier(const RnAccessor &acc) : acc(acc) {}

  sha256_hash_t ComputeChunkHash(int chunk) {
    return ComputeSha256(GetChunkBytes(chunk));
  }

private:
  byte_span_t GetChunkBytes(int chunk) {
    return byte_span_t(acc.map.data() + chunk_byte_size * chunk, chunk_byte_size);
  }

  const RnAccessor &acc;
};

namespace {

// SHA256 checksums of chunks of r4.bin
constexpr std::pair<int, const char*> r4_chunk_hashes[] = {
  // First three chunks
  {0, "e56a04df6ec6e61b03e651247929c8e99048c87274d0582abfb868cf7ba10fe4"},
  {1, "440024667100b0aed051b067ed089ccb90b82b2bc24f9e0fa6d7f1eb7e1f6fe6"},
  {2, "66b67cf63717a57cabe3ce9f1a3ecdea8383feb8a43a6dc39e05f77098963dbe"},

  // Last three chunks
  {7426, "e29eb5ad29401478d4170e92cf3728312672060c72e6d12d813dbb7d8c8f4306"},
  {7427, "118f46577b86d2623363ee1f076854bc064e6700288cdc9e22ff25974111705f"},
  {7428, "49e6d3ffd64e5bf03ad08fae0299cf0f889458a2c245316722deed534feb0243"},

  // Chunks that were wrongly calculated in an old version of r4.bin :-(
  {2436, "66a4effeb5b08bd0a943095b721c55a323d6c808b2f3a7981f5d0a16482a42c9"},
  {2500, "9c23655ae2783e7fd83a609b6bb611765c776942acaba516bd93f5039f2f72c7"},
  {3603, "ab109a5e233114d49ca0110f5769c5f5107d14b5ac063d394683e733507b951b"},
  {4898, "a2e7804978dc9048b60d5acb41eece77dfe0766512bbedd358b409c111a55bf9"},
  {5824, "1d203d24cc6fb9ffbc606678b321740418293e2bb7a1da141e8bec86287eeae6"},

  // Chunk that was possibly corrupted on 1 machine during phase 5?
  // See incidents/phase-5.txt for details.
  {2671, "797d266e799544af8cfe14488ec1b417fc0a7a1da52b0676eb1f94497a64c0db"},
};

// SHA256 checksums of chunks of r5.bin
constexpr std::pair<int, const char*> r5_chunk_hashes[] = {
  // First three chunks
  {0, "d7ad317d97009ddb012507c976a60250f8f7cef9365357c3c1777a2655e8d8f1"},
  {1, "2abe770a37785eb26cdf845da7b5941743b8cc1043bd745f8008322d1ce6549c"},
  {2, "1650ccb1121b9f94e460a1439134ea300f36883ac412592db8f0d0c440e6b326"},

  // Last three chunks
  {7426, "5089c320c9205a2cbbc847b4ee213b7e12e3a3e5c42c4b15134194772c650af1"},
  {7427, "928f18274ae25c4f240b9d36eeb82a741436a1da8655654f3d505ab8ac05a635"},
  {7428, "5a9c5fa04867ada2e217457a00a63a0e7d1b2054ddef0afbf53b8112d766cd31"},

  // A few random chunks in the middle.
  {1486, "5c780eb878f45676f3379c9e6711213ad4a8414a9253d3e200b6dcf52d8cff6a"},
  {2972, "729fd279e03c729015cd63302262aa7ce4cdddd4d9384f2ee5134dc924230884"},
  {4457, "1d21fcb35a9dbb151421885cab2fc6e95eab0d134ab28a5c9ca1974502773fc0"},
};

// SHA256 checksums of chunks of r6.bin
constexpr std::pair<int, const char*> r6_chunk_hashes[] = {
  // First three chunks
  {0, "1d2f0ebdf9b526403f89f93ecf6df7aa4f23a331a6666b5e15df2b21d31a75b3"},
  {1, "0c3e24d42543ae625b5c984c078b09474abd6ceb1452340eac20eff8d35eca17"},
  {2, "9f12bf1e86a926e66da3a060befee3bc70fe2c364cd6d3e40f557fb3a53781c6"},

  // Last three chunks
  {7426, "9fa40121068806a26c171f70097d178810ef346d71f0da17de5f66b03c87c01a"},
  {7427, "bc6738905085221b1abd426d2fd0a06e02df626add1476e1a220af340035fbaa"},
  {7428, "cbbf6d773e15196e5a1cc6fce46ce0aad813cc1be7a14f531afd72d0baa7e370"},

  // A few random chunks in the middle.
  {1486, "7414ac59aa9b3a41c362403ef6f4619a6ea722781e55dbc2ba187efd42a15d80"},
  {2972, "b7dd63f3338014cf70377da5cae5a60f793c7d0c4806c1d22d2768143d38dfea"},
  {4457, "2bcec64447633a63a6ca19dfe6b467560554eb398e04127b2e9cc786b816b0d0"},
};

std::random_device dev;
std::mt19937 rng(dev());

int Verify(int phase, const RnAccessor &acc, const std::pair<int, const char*> *hashes, size_t size) {
  ChunkVerifier verifier = ChunkVerifier(acc);
  int failures = 0;
  for (size_t i = 0; i < size; ++i) {
    int chunk = hashes[i].first;
    std::optional<sha256_hash_t> expected_hash = HexDecode(hashes[i].second);
    if (!expected_hash) {
      std::cerr << "Couldn't decode expected hash for "
          << "phase " << phase << " chunk " << chunk << "!" << std::endl;
      ++failures;
    } else {
      sha256_hash_t computed_hash = verifier.ComputeChunkHash(chunk);
      if (computed_hash != *expected_hash) {
        std::cerr << "Verification of phase " << phase << " chunk " << chunk << " failed!\n"
            << "Expected SHA-256 sum: " << HexEncode(*expected_hash) << "\n"
            << "Computed SHA-256 sum: " << HexEncode(computed_hash) << std::endl;
        ++failures;
      }
    }
  }
  return failures;
}

template<int N>
int Verify(int phase, const RnAccessor &acc, const std::pair<int, const char*> (&hashes)[N]) {
  return Verify(phase, acc, hashes, N);
}

std::optional<std::vector<sha256_hash_t>> LoadChecksums(const char *filename) {
  std::vector<sha256_hash_t> result;
  std::ifstream ifs(filename);  // open in text mode
  if (!ifs) {
    std::cerr << "Could not open " << filename << std::endl;
    return {};
  }
  std::string line;
  for (int i = 0; std::getline(ifs, line); ++i) {
    std::istringstream iss(line);
    std::string encoded_hash;
    iss >> encoded_hash;
    std::optional<sha256_hash_t> decoded_hash = HexDecode(encoded_hash);
    if (!decoded_hash) {
      std::cerr << "Failed to parse checksum on line " << (i + 1) << " of file " << filename << std::endl;
      return {};
    }
    result.push_back(std::move(*decoded_hash));
  }
  return result;
}

std::string GetChecksumFilename(const char *subdir, int phase) {
  std::ostringstream oss;
  oss << subdir;
  if (*subdir) oss << '/';
  oss << "chunk-r" << phase << ".sha256sum";
  return oss.str();
}

int VerifyChecksums(
    int phase, const RnAccessor &acc,
    const std::vector<sha256_hash_t> &checksums,
    const std::vector<int> chunks) {
    int failures = 0;
  ChunkVerifier verifier = ChunkVerifier(acc);
  int i = 0;
  for (int chunk : chunks) {
    sha256_hash_t computed_hash = verifier.ComputeChunkHash(chunk);
    const sha256_hash_t &expected_hash = checksums[chunk];
    if (computed_hash != expected_hash) {
      std::cerr << "Verification of phase " << phase << " chunk " << chunk << " failed!\n"
          << "Expected SHA-256 sum: " << HexEncode(expected_hash) << "\n"
          << "Computed SHA-256 sum: " << HexEncode(computed_hash) << std::endl;
      ++failures;
    }
    if (chunks.size() > 10 && ++i % 10 == 0) {
      std::cerr << "Verified checksum for phase " << phase << " chunk " << chunk << " (" << i << " of " << chunks.size() << ")...\n";
    }
  }
  return failures;
}

int VerifyFromChecksumFile(int phase, const RnAccessor &acc, int chunks_to_verify) {
  std::string path = GetChecksumFilename("metadata", phase);
  if (!std::filesystem::exists(path)) {
    std::cerr << "Checksum file found for phase " << phase << " does not exist: " << path << std::endl;
    return 1;
  }
  auto checksums = LoadChecksums(path.c_str());
  if (!checksums) {
    std::cerr << "Could not load checksum file." << std::endl;
    return 1;
  }
  if (checksums->size() != num_chunks) {
    std::cerr << "Invalid number of checksums. "
        << "Expected " << num_chunks << ", actual " << checksums->size() << std::endl;
    return 1;
  }

  // Select chunks to verify. Always start with first and last chunk.
  std::vector<int> chunks;
  if (chunks_to_verify > 0) {
    chunks.push_back(0);
  }
  if (chunks_to_verify > 1) {
    chunks.push_back(num_chunks - 1);
  }
  if (chunks_to_verify > 2) {
    for (int i = 1; i < num_chunks - 1; ++i) {
      chunks.push_back(i);
    }
    assert(chunks.size() == num_chunks);
    if (chunks_to_verify < num_chunks) {
      // Randomly choose the remaining chunks.
      std::shuffle(&chunks[2], &chunks[num_chunks], rng);
      chunks.resize(chunks_to_verify);
      std::sort(&chunks[2], &chunks[chunks_to_verify]);
    }
  }
  return VerifyChecksums(phase, acc, *checksums, chunks);
}

}  // namespace

int VerifyInputChunks(int phase, const RnAccessor &acc, int chunks_to_verify) {
  switch (phase) {
    case 4: return Verify(4, acc, r4_chunk_hashes);
    case 5: return Verify(5, acc, r5_chunk_hashes);
    case 6: return Verify(6, acc, r6_chunk_hashes);
    default:
      return VerifyFromChecksumFile(phase, acc, chunks_to_verify);
  }
}
