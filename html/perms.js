'use strict';

const L = 26;

const totalPerms = 401567166000;

const Perm = {
  EMPTY:   0,
  MOVER1:  1,
  PUSHER1: 2,
  MOVER2:  3,
  PUSHER2: 4,
  ANCHOR2: 5,
};

function generateFactorials() {
  const fac = [1, 1];
  for (let i = 2; i < 18; ++i) fac.push(fac[i - 1]*i);
  return fac;
}

// fac[i] = factorial of i = product from 1 through i (inclusive)
const fac = Object.freeze(generateFactorials());

function createMultidimensionalArray(bounds, i) {
  if (i === bounds.length) {
    return 0;
  }
  let res = [];
  for (let j = 0; j < bounds[i]; ++j) {
    res.push(createMultidimensionalArray(bounds, i + 1));
  }
  return res;
}

// numPerms[a][b][c][d][e][f] == number of permutations of a string with a 0s, b 1s, etc.
const numPerms = createMultidimensionalArray([17, 3, 4, 3, 3, 2], 0);

// indexOf_memo[x][a][b][c][d][e] == number of permutations of a string with a 0s, b 1s, etc.
// that have a starting character strictly smaller than x.
const indexOf_memo = createMultidimensionalArray([6, 17, 3, 4, 3, 3, 2], 0);

function indexOfPerm(p) {
  let f = [0, 0, 0, 0, 0, 0];
  let idx = 0;
  for (let i = L - 1; i >= 0; --i) {
    let x = p[i];
    ++f[x];
    idx += indexOf_memo[x][f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
  }
  return idx;
}

function permAtIndex(index) {
  if (!Number.isInteger(index) || index < 0 || index > totalPerms) {
    return undefined;
  }
  const f = [16, 2, 3, 2, 2, 1];
  let p = [];
  for (let i = 0; i < L; ++i) {
    // Note: this could use binary search instead (using the indexOf_memo),
    // but not sure if it's useful in practice, since log(6)/log(2) = ~2.58
    // and this loop runs only 3 iterations on average (6 worst case).
    for (let x = 0; x < 6; ++x) {
      if (f[x] > 0) {
        --f[x];
        const n = numPerms[f[0]][f[1]][f[2]][f[3]][f[4]][f[5]];
        if (n > index) {
          p[i] = x;
          break;
        }
        ++f[x];
        index -= n;
      }
    }
  }
  return index === 0 ? p : undefined;
}

// Precalculate number of permutations.
for (let a = 0; a <= 16; ++a) {
  for (let b = 0; b <= 2; ++b) {
    for (let c = 0; c <= 3; ++c) {
      for (let d = 0; d <= 2; ++d) {
        for (let e = 0; e <= 2; ++e) {
          for (let f = 0; f <= 1; ++f) {
            let n = 1;
            for (let i = a + 1; i <= a + b + c + d + e + f; ++i) n *= i;
            let m = fac[b] * fac[c] * fac[d] * fac[e] * fac[f];
            if(n % m !== 0) alert('Assert failed! ' + n + ' % ' + m + ' !== 0!');
            numPerms[a][b][c][d][e][f] = n / m;
          }
        }
      }
    }
  }
}

// Precalculate number of smaller substrings used by IndexOf.
for (let a = 0; a <= 16; ++a) {
  for (let b = 0; b <= 2; ++b) {
    for (let c = 0; c <= 3; ++c) {
      for (let d = 0; d <= 2; ++d) {
        for (let e = 0; e <= 2; ++e) {
          for (let f = 0; f <= 1; ++f) {
            for(let x = 0; x < 6; ++x) {
              const freq = [a, b, c, d, e, f];
              let n = 0;
              for (let y = 0; y < x; ++y) {
                if (freq[y] > 0) {
                  --freq[y];
                  n += numPerms[freq[0]][freq[1]][freq[2]][freq[3]][freq[4]][freq[5]];
                  ++freq[y];
                }
              }
              indexOf_memo[x][a][b][c][d][e][f] = n;
            }
          }
        }
      }
    }
  }
}

if (numPerms[0][0][0][0][0][0] !== 1 || numPerms[16][2][3][2][2][1] !== totalPerms) {
  alert('Assert failed!');
}
