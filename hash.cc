#include "hash.h"

#include <openssl/sha.h>

static_assert(SHA256_DIGEST_LENGTH == 32);
static_assert(sizeof(sha256_hash_t) == 32);

void ComputeSha256(const uint8_t *data, size_t size, sha256_hash_t &hash) {
  SHA256(data, size, hash.data());
}
