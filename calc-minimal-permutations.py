#!/usr/bin/env python3

# Calculates the total number of reachable states, with or without rotational
# symmetry.
#
# This should print:
#
#  172416263040 (number of reachable states, which matches the
#  brute-forced result in results/count-unreachable-output.txt)
#
#  86208131520 (number of reachable states with rotational
#  symmetry removed; this is just half of the previous number)

def fac(n):
  '''Calculates n!, the factorial of n.'''
  assert n >= 0
  return 1 if n < 2 else n * fac(n - 1)

total = 0
for axes in [
  # Number of axes (horizontal/vertical) that the anchored piece
  # could have pushed in.
        0, 1, 1, 1, 0,
  0, 1, 2, 2, 2, 2, 2, 0,
  0, 2, 2, 2, 2, 2, 1, 0,
     0, 1, 1, 1, 0,
]:
  if axes >= 1:
    # Two possible horizontal patterns:
    #
    #  aY.  .Ya
    #
    # where `a` is any of the remaining pieces. This leaves a total of 23
    # fields with 15 spaces and 8 remaining pieces.
    #
    total += 2*(
      fac(23) // fac(15) // fac(2) // fac(2) // fac(2) // fac(2) +
      fac(23) // fac(15) // fac(3) // fac(1) // fac(2) // fac(2) +
      fac(23) // fac(15) // fac(3) // fac(2) // fac(1) // fac(2) +
      fac(23) // fac(15) // fac(3) // fac(2) // fac(2) // fac(1))

  if axes >= 2:
    # Two possible vertical patterns, with exactly 1 occupied neighbor:
    #
    #   a    .
    #  .Y.  .Y.
    #   .    a
    #
    # This leaves a total of 21 fields with 13 spaces and 8 remaining pieces.
    total += 2*(
      fac(21) // fac(13) // fac(2) // fac(2) // fac(2) // fac(2) +
      fac(21) // fac(13) // fac(3) // fac(1) // fac(2) // fac(2) +
      fac(21) // fac(13) // fac(3) // fac(2) // fac(1) // fac(2) +
      fac(21) // fac(13) // fac(3) // fac(2) // fac(2) // fac(1))
    # Two possible vertical patterns, in addition to the above
    # horizontal patterns:
    #
    #    a    .
    #   bYc  aYb
    #    .    c
    #
    # Where (a,b,c) can be any combination or remaining pieces: 61 out of
    # 64 possible combinations, because all three pieces van be white pushers
    # (of which there are 3) but not three white moves, black pushers or
    # black movers (of which are are only 2). In each case, we are left with
    # 21 fields with 15 spaces and 6 remaining pieces.
    for a in range(4):
      for b in range(4):
        for c in range(4):
          p = 3 - (a == 0) - (b == 0) - (c == 0)
          q = 2 - (a == 1) - (b == 1) - (c == 1)
          r = 2 - (a == 2) - (b == 2) - (c == 2)
          s = 2 - (a == 3) - (b == 3) - (c == 3)
          if p >= 0 and q >= 0 and r >= 0 and s >= 0:
            total += 2*(fac(21) // fac(15) // fac(p) // fac(q) // fac(r) // fac(s))

    # Note that cases with exactly 1 horizontal neighbor, like for example:
    #
    #    a          a
    #   bY.   or   .Yb
    #    .          c
    #
    # would have been covered already by the horizontal patterns above, so they
    # should not be counted again.

# Total reachable permutations.
print(total)

# Total reachable permutations up to rotation (e.g., orient the board so the
# anchor is in the top half).
print(total // 2)
