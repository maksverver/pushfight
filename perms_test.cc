#include "perms.h"

#include "board.h"
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

  assert(IsValid(first_perm));
  assert(IsValid(last_perm));

  assert(PermAtIndex(0) == first_perm);
  assert(PermAtIndex(total_perms - 1) == last_perm);

  assert(IndexOf(first_perm) == 0);
  assert(IndexOf(last_perm) == total_perms - 1);

  REP(n, 100) {
    std::uniform_int_distribution<int64_t> dist(0, total_perms - 1000);
    int64_t idx = dist(rng);
    Perm perm = PermAtIndex(idx);
    assert(IsValid(perm));
    REP(m, 1000) {
      assert(IndexOf(perm) == idx + m);
      std::next_permutation(std::begin(perm), std::end(perm));
      assert(IsValid(perm));
    }
  }

  // Rotation
  {
    Perm p = {
            0, 3, 0, 0, 0,
      0, 3, 5, 0, 2, 0, 0, 0,
      0, 0, 0, 2, 0, 4, 0, 1,
         0, 4, 0, 0, 1,
    };
    Perm q = {
            1, 0, 0, 4, 0,
      1, 0, 4, 0, 2, 0, 0, 0,
      0, 0, 0, 2, 0, 5, 3, 0,
         0, 0, 0, 3, 0,
    };
    assert(Rotated(p) == q);
    assert(Rotated(q) == p);
  }

  // Minimal indices (first and last 10).
  REP(i, 10) {
    Perm p = PermAtMinIndex(i);
    assert(IsValid(p));
    assert(MinIndexOf(p) == i);

    p = PermAtMinIndex(min_index_size - 1 - i);
    assert(IsValid(p));
    assert(MinIndexOf(p) == min_index_size - 1 - i);
  }

  // Detecting & undoing rotation.
  {
    Perm p = PermAtMinIndex(1234567890);
    Perm q = Rotated(p);
    assert(PermAtMinIndex(1234567890, true) == q);
    bool rotated = true;
    assert(MinIndexOf(p, &rotated) == 1234567890 && rotated == false);
    assert(MinIndexOf(q, &rotated) == 1234567890 && rotated == true);
  }

  // Random minimal indices
  REP(n, 1000) {
    std::uniform_int_distribution<int64_t> dist(0, min_index_size - 1);
    int64_t idx = dist(rng);
    Perm perm = PermAtMinIndex(idx);
    assert(IsValid(perm));
    assert(MinIndexOf(perm) == idx);
  }

  // Minimal indices of reachable positions.
  REP(n, 1000) {
    std::uniform_int_distribution<int64_t> dist(0, total_perms);
    int64_t idx;
    Perm perm;
    do {
      idx = dist(rng);
      perm = PermAtIndex(idx);
    } while (!IsReachable(perm));
    bool rotated;
    int64_t i = MinIndexOf(perm, &rotated);
    assert(PermAtMinIndex(i, rotated) == perm);
  }
}
