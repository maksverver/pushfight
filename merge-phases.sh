#!/bin/sh

set -e -E -o pipefail

output=output/merged.bin
if [ -e "$output" ]; then
  echo "$output already exists!"
  exit 1
fi

./merge-phases \
  <(zstdcat archives/r0.bin.zst) \
  <(zstdcat archives/r2.bin.zst) \
  <(zstdcat archives/r4.bin.zst) \
  <(zstdcat archives/r6.bin.zst) \
  <(zstdcat archives/r8.bin.zst) \
  <(zstdcat archives/r10.bin.zst) \
  <(zstdcat archives/r12.bin.zst) \
  <(zstdcat archives/r14.bin.zst) \
  <(zstdcat archives/r16.bin.zst) \
  <(zstdcat archives/r18.bin.zst) \
  <(zstdcat archives/r20-new.bin.zst) \
  <(zstdcat archives/r22-new.bin.zst) \
  <(zstdcat archives/r24-new.bin.zst) \
  <(zstdcat archives/r26-new.bin.zst) \
  <(zstdcat archives/r28-new.bin.zst) \
  <(zstdcat archives/r30-new.bin.zst) \
  <(zstdcat archives/r32-new.bin.zst) \
  <(zstdcat archives/r34-new.bin.zst) \
  <(zstdcat archives/r36-new.bin.zst) \
  <(zstdcat archives/r38-new.bin.zst) \
  <(zstdcat archives/r40-new.bin.zst) \
  <(zstdcat archives/r42-new.bin.zst) \
  <(zstdcat archives/r44-new.bin.zst) \
  <(zstdcat archives/r46-new.bin.zst) \
  <(zstdcat archives/r48-new.bin.zst) \
  <(zstdcat archives/r50-new.bin.zst) \
  <(zstdcat archives/r52-new.bin.zst) \
  <(zstdcat archives/r54-new.bin.zst) \
  <(zstdcat archives/r56-new.bin.zst) \
  <(zstdcat archives/r58-new.bin.zst) \
  <(zstdcat archives/r60-new.bin.zst) \
  <(zstdcat archives/r62-new.bin.zst) \
  <(zstdcat archives/r64-new.bin.zst) \
  <(zstdcat archives/r66-new.bin.zst) \
  <(zstdcat archives/r68-new.bin.zst) \
  <(zstdcat archives/r70-new.bin.zst) \
  <(zstdcat archives/r72-new.bin.zst) \
  <(zstdcat archives/r74-new.bin.zst) \
  <(zstdcat archives/r76-new.bin.zst) \
  <(zstdcat archives/r78-new.bin.zst) \
  <(zstdcat archives/r80-new.bin.zst) \
  <(zstdcat archives/r82-new.bin.zst) \
  <(zstdcat archives/r84-new.bin.zst) \
  <(zstdcat archives/r86-new.bin.zst) \
  <(zstdcat archives/r88-new.bin.zst) \
  <(zstdcat archives/r90-new.bin.zst) \
  <(zstdcat archives/r92-new.bin.zst) \
  <(zstdcat archives/r94-new.bin.zst) \
  <(zstdcat archives/r96-new.bin.zst) \
  <(zstdcat archives/r98-new.bin.zst) \
  2> output/merged-frequencies.txt \
  | zstd -19 -o output/merged.bin.zst
