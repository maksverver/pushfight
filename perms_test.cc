#include "perms.h"

#include "macros.h"

#ifdef NDEBUG
#error "Can't compile test with -DNDEBUG!"
#endif
#include <assert.h>

#include <algorithm>
#include <iostream>
#include <random>

namespace {

std::random_device dev;
std::mt19937 rng(dev());

}  // namespace

int main() {
  InitializePerms();

  assert(PermAtIndex(0) == first_perm);
  assert(PermAtIndex(total_perms - 1) == last_perm);

  assert(IndexOf(first_perm) == 0);
  assert(IndexOf(last_perm) == total_perms - 1);
  REP(n, 100) {
    std::uniform_int_distribution<int64_t> dist(0, total_perms - 1000);
    int64_t idx = dist(rng);
    Perm perm = PermAtIndex(idx);
    REP(m, 1000) {
      assert(IndexOf(perm) == idx + m);
      std::next_permutation(&perm[0], &perm[26]);
    }
  }
}
