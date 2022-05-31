#!/bin/sh

# Extract a chunk from a ternary file.

set -e

if [ $# != 2 ]; then
  echo 'Usage: extract-rN-chunk <rN.bin> <chunk>' >&2
  exit 0
fi

filename=$1
chunk=$2

if ! test -r "$filename"; then
    echo "$filename is not readable" >&2
    exit 1
fi

if [ "$chunk" -lt 0 ] || [ "$chunk" -ge 7429 ]; then
    echo "Invalid chunk number: $chunk" >&2
    exit 1
fi

if test -t 1; then
    echo 'Standard output is a terminal! Aborting.' >&2
    exit 1
fi

tail -c +$((10810800 * $chunk + 1)) "$filename" | head -c 10810800
