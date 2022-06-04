#include "input-verification.h"

#include <iostream>

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

}  // namespace

int VerifyInputChunks(int phase, const RnAccessor &acc) {
  switch (phase) {
    case 4: return Verify(4, acc, r4_chunk_hashes);
    case 5: return Verify(5, acc, r5_chunk_hashes);

    default:
      std::cerr << "WARNING: no input file verification data exists for phase " << phase << "!";
      return 0;
  }
}
