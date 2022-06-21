#include "perms.h"

#include "macros.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>

namespace {

static_assert(L == 26);

// num_perms[a][b][c][d][e][f] == number of permutations of a string with a 0s, b 1s, etc.
int64_t num_perms[17][3][4][3][3][2];

// fac[i] = factorial of i = product from 1 through i (inclusive)
int64_t fac[21];

// indexOf_memo[x][a][b][c][d][e] == number of permutations of a string with a 0s, b 1s, etc.
// that have a starting character strictly smaller than x.
int64_t indexOf_memo[6][17][3][4][3][3][2];

// Calculates index of a permutation.
//
// Elements must be added from back to front!
struct IndexOfCalculator {
  std::array<int, 6> f = {};
  int64_t idx = 0;

  void Add(int x) {
    ++f[x];
    idx += indexOf_memo[x][f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
  }

  void Add(const char *begin, const char *end) {
    while (begin < end) Add(*--end);
  }
};

int64_t IndexOfImpl(const char *begin, const char *end) {
  IndexOfCalculator calc;
  calc.Add(begin, end);
  return calc.idx;
}

void PermAtIndexImpl(int64_t idx, std::array<int, 6> &f, char *begin, char *end) {
  for (char *p = begin; p < end; ++p) {
    // Note: this could use binary search instead (using the indexOf_memo),
    // but not sure if it's useful in practice, since log(6)/log(2) = ~2.58
    // and this loop runs only 3 iterations on average (6 worst case).
    int x;
    for (x = 0; x < 6; ++x) if (f[x] > 0) {
      --f[x];
      int64_t n = num_perms[f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
      if (n > idx) break;
      ++f[x];
      idx -= n;
    }
    assert(x < 6);
    *p = x;
  }
  assert(idx == 0);
}

// Minimized index axes for the anchor piece.
//
// If 0, the anchor cannot be placed there.
// If 1, the anchor can only move horizontally.
// If 2, the anchor can move horizontally or vertically.
constexpr int axes[13] = {
//        0  1  2  3  4
          0, 1, 1, 1, 0,
    0, 1, 2, 2, 2, 2, 2, 0,
//  5  6  7  8  9 10 11 12
};

int64_t min_index_horiz_offset_begin[5][5];
int64_t min_index_verti_offset_begin[5][5][5][5];  // top, left, right, bottom
// min_index_start[i] = start index for permutations with the anchor at field i
int64_t min_index_anchor_offset_begin[13];

// Ends used by PermAtMinIndex().
int64_t min_index_horiz_offset_end[5][5];
int64_t min_index_verti_offset_end[5][5][5][5];
int64_t min_index_anchor_offset_end[13];

Perm PermAtMinIndexImpl(int64_t idx) {
  assert(idx >= 0 && idx < min_index_size);

  Perm perm;
  REP(i, 13) if (idx < min_index_anchor_offset_end[i]) {

    idx -= min_index_anchor_offset_begin[i];

    assert(axes[i] > 0 && i >= 1);

    REP(a, 5) REP(b, 5) if (idx < min_index_horiz_offset_end[a][b]) {
      idx -= min_index_horiz_offset_begin[a][b];

      perm[i - 1] = a;
      perm[i    ] = 5;
      perm[i + 1] = b;

      std::array<int, 6> f = all_freq;
      --f[a];
      --f[5];
      --f[b];
      char remaining[23];
      PermAtIndexImpl(idx, f, std::begin(remaining), std::end(remaining));
      int pos = 0;
      FOR(j,     0, i - 1) perm[j] = remaining[pos++];
      FOR(j, i + 2,     L) perm[j] = remaining[pos++];
      assert(pos == 23);
      goto done;
    }

    assert(axes[i] > 1 && i >= 7);

    REP(a, 5) REP(b, 5) REP(c, 5) REP(d, 5) if (idx < min_index_verti_offset_end[a][b][c][d]) {
      idx -= min_index_verti_offset_begin[a][b][c][d];

      perm[i - 7] = a;
      perm[i - 1] = b;
      perm[i    ] = 5;
      perm[i + 1] = c;
      perm[i + 8] = d;

      std::array<int, 6> f = all_freq;
      --f[a];
      --f[b];
      --f[5];
      --f[c];
      --f[d];
      char remaining[21];
      PermAtIndexImpl(idx, f, std::begin(remaining), std::end(remaining));
      int pos = 0;
      FOR(j,     0, i - 7) perm[j] = remaining[pos++];
      FOR(j, i - 6, i - 1) perm[j] = remaining[pos++];
      FOR(j, i + 2, i + 8) perm[j] = remaining[pos++];
      FOR(j, i + 9,     L) perm[j] = remaining[pos++];
      assert(pos == 21);
      goto done;
    }
  }
  assert(false);

done:
  return perm;
}

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

  // Calculate minimized index offsets.
  // See calc-minimal-permutations.py for the logic here.
  {
    int64_t horiz = 0;
    REP(a, 5) REP(b, 5) {
      min_index_horiz_offset_begin[a][b] = horiz;
      if (a == 0 && b != 0) {
        // .Yb
        horiz += num_perms[15][2 - (b == 1)][3 - (b == 2)][2 - (b == 3)][2 - (b == 4)][0];
      }
      if (a != 0 && b == 0) {
        // aY.
        horiz += num_perms[15][2 - (a == 1)][3 - (a == 2)][2 - (a == 3)][2 - (a == 4)][0];
      }
      min_index_horiz_offset_end[a][b] = horiz;
    }
    int64_t verti = horiz;
    REP(a, 5) REP(b, 5) REP(c, 5) REP(d, 5) {
      min_index_verti_offset_begin[a][b][c][d] = verti;
      if (a == 0 && b == 0 && c == 0 && d != 0) {
        //  .
        // .Y.
        //  d
        verti += num_perms[13][2 - (d == 1)][3 - (d == 2)][2 - (d == 3)][2 - (d == 4)][0];
      }
      if (a != 0 && b == 0 && c == 0 && d == 0) {
        //  a
        // .Y.
        //  .
        verti += num_perms[13][2 - (a == 1)][3 - (a == 2)][2 - (a == 3)][2 - (a == 4)][0];
      }
      if (a != 0 && b != 0 && c != 0 && d == 0) {
        int p = 2 - (a == 1) - (b == 1) - (c == 1);
        int q = 3 - (a == 2) - (b == 2) - (c == 2);
        int r = 2 - (a == 3) - (b == 3) - (c == 3);
        int s = 2 - (a == 4) - (b == 4) - (c == 4);
        if (p >= 0 && q >= 0 && r >= 0 && s >= 0) {
          //  a
          // bYc
          //  .
          verti += num_perms[15][p][q][r][s][0];
        }
      }
      if (a == 0 && b != 0 && c != 0 && d != 0) {
        int p = 2 - (b == 1) - (c == 1) - (d == 1);
        int q = 3 - (b == 2) - (c == 2) - (d == 2);
        int r = 2 - (b == 3) - (c == 3) - (d == 3);
        int s = 2 - (b == 4) - (c == 4) - (d == 4);
        if (p >= 0 && q >= 0 && r >= 0 && s >= 0) {
          //  .
          // bYc
          //  d
          verti += num_perms[15][p][q][r][s][0];
        }
      }
      min_index_verti_offset_end[a][b][c][d] = verti;
    }
    int64_t total = 0;
    REP(i, 13) {
      min_index_anchor_offset_begin[i] = total;
      if (axes[i] == 1) {
        total += horiz;
      } else if (axes[i] == 2) {
        total += verti;
      }
      min_index_anchor_offset_end[i] = total;
    }
    assert(total == min_index_size);
  }
}

bool IsValid(const Perm &perm) {
  std::array<int, 6> f = all_freq;
  for (int x : perm) {
    if (x < 0 || x >= 6 || f[x] == 0) return false;
    --f[x];
  }
  return true;
}

int64_t IndexOf(const Perm &p) {
  return IndexOfImpl(p.begin(), p.end());
}

Perm PermAtIndex(int64_t idx) {
  assert(idx >= 0 && idx < total_perms);
  std::array<int, 6> f = all_freq;
  Perm perm;
  PermAtIndexImpl(idx, f, perm.begin(), perm.end());
  return perm;
}

void Rotate(Perm &perm) {
  std::reverse(perm.begin(), perm.end());
}

Perm Rotated(const Perm &perm) {
  Perm result;
  REP(i, L) result[i] = perm[L - 1 - i];
  return result;
}

int64_t MinIndexOf(const Perm &p, bool *rotated) {
  REP(i, 26) if (p[i] == 5) {
    if (i >= 13) {
      if (rotated) *rotated = true;
      return MinIndexOf(Rotated(p));
    }
    assert(axes[i] > 0);
    int64_t offset = min_index_anchor_offset_begin[i];
    if ((p[i - 1] == 0) != (p[i + 1] == 0)) {
      offset += min_index_horiz_offset_begin[int{p[i - 1]}][int{p[i + 1]}];
      IndexOfCalculator calc;
      calc.Add(&p[i + 2], &p[L]);
      calc.Add(&p[0], &p[i - 1]);
      offset += calc.idx;

    } else {
      assert(axes[i] == 2);
      offset += min_index_verti_offset_begin[int{p[i - 7]}][int{p[i - 1]}][int{p[i + 1]}][int{p[i + 8]}];
      IndexOfCalculator calc;
      calc.Add(&p[i + 9], &p[L]);
      calc.Add(&p[i + 2], &p[i + 8]);
      calc.Add(&p[i - 6], &p[i - 1]);
      calc.Add(&p[0    ], &p[i - 7]);
      offset += calc.idx;
    }
    if (rotated) *rotated = false;
    return offset;
  }
  assert(false);
  return -1;
}

Perm PermAtMinIndex(int64_t idx, bool rotated) {
  Perm perm = PermAtMinIndexImpl(idx);
  if (rotated) Rotate(perm);
  return perm;
}
