#include "hash.h"

#include <optional>
#include <string>
#include <string_view>

#include <openssl/sha.h>

static_assert(SHA256_DIGEST_LENGTH == 32);
static_assert(sizeof(sha256_hash_t) == 32);

void ComputeSha256(const uint8_t *data, size_t size, sha256_hash_t &hash) {
  SHA256(data, size, hash.data());
}

std::string HexEncode(sha256_hash_t hash) {
  static const char *digits = "0123456789abcdef";
  std::string result;
  result.reserve(hash.size() * 2);
  for (uint8_t byte : hash) {
    result += digits[(byte >> 4) & 0xf];
    result += digits[(byte >> 0) & 0xf];
  }
  return result;
}

static int ParseHexDigit(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

std::optional<sha256_hash_t> HexDecode(std::string_view s) {
  sha256_hash_t result;
  if (s.size() != 2 * result.size()) return {};
  for (size_t i = 0; i < result.size(); ++i) {
    int hi = ParseHexDigit(s[2*i + 0]);
    int lo = ParseHexDigit(s[2*i + 1]);
    if (hi < 0 || lo < 0) return {};
    result[i] = (hi << 4) | lo;
  }
  return result;
}
