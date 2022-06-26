#!/bin/sh

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
  <(zstdcat archives/r68-new.bin.zst)
