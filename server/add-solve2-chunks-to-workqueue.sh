#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
counts=../metadata/r$(expr $phase - 2)-counts.txt

if (( $phase % 2 )); then
  echo "Phase must be even!"
  exit 1
fi

if ! test -f "$counts"; then
  echo "Missing input counts: $counts"
  exit 1
fi

cat "$counts" | {
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
