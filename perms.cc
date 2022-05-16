#include "perms.h"

#include "macros.h"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

// num_perms[a][b][c][d][e][f] == number of permutations of a string with a 0s, b 1s, etc.
int64_t num_perms[17][3][4][3][3][2];

// fac[i] = factorial of i = product from 1 through i (inclusive)
int64_t fac[21];

// indexOf_memo[x][a][b][c][d][e] == number of permutations of a string with a 0s, b 1s, etc.
// that have a starting character strictly smaller than x.
int64_t indexOf_memo[6][17][3][4][3][3][2];

}  // namespace

void InitializePerms() {
  // Precalculate factorials up to 20.
  fac[0] = fac[1] = 1;
  FORE(i, 2, 20) fac[i] = i * fac[i - 1];

  // Precalculate number of permutations.
  REPE(a, 16) REPE(b, 2) REPE(c, 3) REPE(d, 2) REPE(e, 2) REPE(f, 1) {
    int64_t n = 1;
    FORE(i, a + 1, a + b + c + d + e + f) n *= i;
    int64_t m = fac[b] * fac[c] * fac[d] * fac[e] * fac[f];
    //std::cout << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << ": " << n << '/' << m << std::endl;
    assert(n % m == 0);
    num_perms[a][b][c][d][e][f] = n / m;
  }
  assert(num_perms[0][0][0][0][0][0] == 1);
  assert(num_perms[16][2][3][2][2][1] == total_perms);

  // Precalculate number of smaller substrings used by IndexOf.
  REPE(a, 16) REPE(b, 2) REPE(c, 3) REPE(d, 2) REPE(e, 2) REPE(f, 1) REP(x, 6) {
    std::array<int, 6> freq = {a, b, c, d, e, f};
    int64_t n = 0;
    REP(y, x) {
      if (freq[y] > 0) {
        --freq[y];
        n += num_perms[freq[0]][freq[1]][freq[2]][freq[3]][freq[4]][freq[5]];
        ++freq[y];
      }
    }
    indexOf_memo[x][a][b][c][d][e][f] = n;
  }
}

int64_t IndexOf(const Perm &p) {
  std::array<int, 6> f = {};
  int64_t idx = 0;
  REPD(i, L) {
    int x = p[i];
    ++f[x];
    idx += indexOf_memo[x][f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
  }
  return idx;
}

Perm PermAtIndex(int64_t idx) {
  assert(idx >= 0 && idx < total_perms);
  std::array<int, 6> f = all_freq;
  Perm p;
  REP(i, L) {
    // Note: this could use binary search instead (using the indexOf_memo),
    // but not sure if it's useful in practice.
    REP(x, 6) if (f[x] > 0) {
      --f[x];
      int64_t n = num_perms[f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
      //std::cout << i << ' ' << x << ' ' << f[0] << ' ' << f[1] << ' ' << f[2] << ' ' << f[3] << ' ' << f[4] << ' ' << f[5] << ' ' << f[0] << ' ' << ' ' << n << std::endl;
      if (n > idx) {
        p[i] = x;
        break;
      }
      ++f[x];
      idx -= n;
    }
  }
  assert(idx == 0);
  return p;
}
