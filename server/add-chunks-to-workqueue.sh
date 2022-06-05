#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
input=../input/r$(expr $phase - 1).bin

if ! test -f "$input"; then
    echo "Missing input file: $input"
    exit 1
fi

../count-r1 "$input" | {
  echo 'BEGIN;'

  read line  # skip header line
  while read chunk ties losses wins total ; do
    if [ $chunk -ge 0 ]; then
      echo "$chunk $ties" >&2
      # Note: we could omit solved chunks (without ties) from the work queue,
      # but then stitching the results back together becomes more complicated
      # since we need to merge existing blocks from the previous bin file
      # instead of simply concatenating the results. Let's not bother with it
      # while the percentage of solved chunks is only ~10%.
      #if [ $ties -gt 0 ]; then
        echo "INSERT INTO WorkQueue (phase, chunk, difficulty) VALUES ($phase, $chunk, $ties);"
      #fi
    fi
  done

  echo 'COMMIT;'

} | sqlite3 chunks.db
