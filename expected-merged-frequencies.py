#!/usr/bin/python3

# Tool to calculate the expected frequencies in the output from
# merge-phases (by chunk).
#
# The output is a series of three numbers:
#
#  chunk, value, count
#
# Where `value` is the byte value stored by merge-phases (i.e., 0 = tie,
# 1 = loss in 0, 2 = win in 1, 3 = loss in 1, etc.), and `count` is the
# number of such bytes in the given merged output chunk.
#
# Only lines where count > 0 are included in the output.

from itertools import groupby
import os
import sys

num_chunks = 7429
chunk_size = 54054000

phase_paths= []
for filename in os.listdir('metadata'):
  if filename.startswith('r') and filename.endswith('-counts.txt'):
    phase = int(filename[1:-11])
    path = os.path.join('metadata', filename)
    phase_paths.append((phase, path))
phase_paths.sort()

value_counts_per_chunk = [[] for _ in range(num_chunks)]

last_phase = 0
last_losses_per_chunk = [0]*num_chunks
last_wins_per_chunk = [0]*num_chunks

# Immediately-lost positions (value 1)
for chunk, indices in groupby(map(int, open('results/lost-perm-ids.txt','rt')), lambda index: index // chunk_size):
  count = len(list(indices))
  value_counts_per_chunk[chunk].append((1, count))
  last_losses_per_chunk[chunk] += count

for phase, path in phase_paths:
  assert 1 <= phase - last_phase <= 2
  lines = list(open(path, 'rt'))
  assert len(lines) == 7431
  assert lines[0].split() == ['Chunk', 'Ties', 'Losses', 'Wins', 'Total']
  for chunk in range(num_chunks):
    i, ties, losses, wins, total = map(int, lines[chunk + 1].split())
    assert i == chunk
    assert total == chunk_size
    new_losses = losses - last_losses_per_chunk[chunk]
    new_wins = wins - last_wins_per_chunk[chunk]
    if phase == 1 and new_wins > 0:
      value_counts_per_chunk[chunk].append((2, new_wins))
    if new_losses > 0:
      value = phase + (phase & 1) + 1
      value_counts_per_chunk[chunk].append((value, new_losses))
    if phase > 1 and new_wins > 0:
      assert phase % 2 == 0
      value = phase + 2
      value_counts_per_chunk[chunk].append(((phase | 1) + 1, new_wins))
    last_losses_per_chunk[chunk] = losses
    last_wins_per_chunk[chunk] = wins
  last_phase = phase

for chunk, value_counts in enumerate(value_counts_per_chunk):
  ties = chunk_size - sum(count for (value, count) in value_counts)
  if ties > 0:
    print(chunk, 0, ties)
  for value, count in value_counts:
    print(chunk, value, count)
