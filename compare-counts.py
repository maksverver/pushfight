#!/usr/bin/python3

import sys

if len(sys.argv) != 3:
  print('Usage: compare-counts.py <r1-counts.txt> <r2-counts.txt>')
  sys.exit(1)

lines1 = list(open(sys.argv[1], 'rt'))
lines2 = list(open(sys.argv[2], 'rt'))
assert lines1[0].split() == ['Chunk', 'Ties', 'Losses', 'Wins', 'Total']
assert lines2[0].split() == ['Chunk', 'Ties', 'Losses', 'Wins', 'Total']
counts1 = [list(map(int, line.split())) for line in lines1[1:]]
counts2 = [list(map(int, line.split())) for line in lines2[1:]]

assert len(counts1) == len(counts2)

total_new_wins = 0
total_new_losses = 0
for i in range(len(counts1) - 1):
  chunk1, ties1, losses1, wins1, total1 = counts1[i]
  chunk2, ties2, losses2, wins2, total2 = counts2[i]
  assert chunk1 == chunk2 == i
  assert ties1 + losses1 + wins1 == total1 == ties2 + losses2 + wins2 == total2
  assert ties1 >= ties1 and losses1 <= losses2 and wins1 <= wins2
  total_new_wins += wins2 - wins1
  total_new_losses += losses2 - losses1

chunk1, ties1, losses1, wins1, total1 = counts1[-1]
chunk2, ties2, losses2, wins2, total2 = counts2[-1]
assert chunk1 == chunk2 == -1
assert ties1 + losses1 + wins1 == total1 == ties2 + losses2 + wins2 == total2
assert ties1 >= ties1 and losses1 <= losses2 and wins1 <= wins2
assert wins2 - wins1 == total_new_wins
assert losses2 - losses1 == total_new_losses
print('New losses: ', total_new_losses)
print('New wins:   ', total_new_wins)
print('Total:      ', total_new_losses + total_new_wins)
