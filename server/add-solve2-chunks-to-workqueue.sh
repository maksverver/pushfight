#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
input=../input/r$(expr $phase - 2).bin

if (( $phase % 2 )); then
  echo "Phase must be even!"
  exit 1
fi

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
      if [ $ties -gt 0 ]; then
        echo "INSERT INTO WorkQueue (phase, chunk, difficulty) VALUES ($phase, $chunk, $ties);"
      fi
    fi
  done

  echo 'COMMIT;'

} | sqlite3 chunks.db
