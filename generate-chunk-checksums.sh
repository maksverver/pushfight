#!/bin/sh

# Generates per-chunk checksums from a complete rN.bin file.
#
# Usage:
#
#  ./generate-chunk-checksums.sh 10 | tee metadata/chunk-r10.sha256sum
#
# Note: the current version doesn't support streaming input from stdin, though
# that should be possible too (using `split -b 10810800` instead of
# `split -n 7429`.)

phase=$1

if [ -z "$phase" ]; then
  echo 'Missing phase!'
  exit 1
fi

input=input/r$phase.bin
if [ ! -f "$input" ]; then
  echo "Missing input file: $input"
  exit 1
fi

if [ "$(stat -c %s "$input")" -ne 80313433200 ]; then
  echo "Unexpected input size!"
fi

split -n 7429 "$input" --filter sha256sum | nl -v0 |
while read i hash file; do
  printf "%s  chunk-r%d-%04d.bin\n" "$hash" "$phase" "$i";
done
