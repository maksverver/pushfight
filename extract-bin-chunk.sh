#!/bin/sh

# Extract a chunk from a binary file

set -e

if [ $# != 2 ]; then
  echo 'Usage: extract-bin-chunk <rN.bin> <chunk>'
  exit 0
fi

filename=$1
chunk=$2

if ! test -r "$filename"; then
    echo "$filename is not readable"
    exit 1
fi


if [ "$chunk" -lt 0 ] || [ "$chunk" -ge 7429 ]; then
    echo "Invalid chunk number: $chunk"
fi

if test -t 1; then
    echo 'Standard output is a terminal! Aborting.'
    exit 1
fi

tail -c +$((6756750 * $chunk + 1)) "$filename" | head -c 6756750
