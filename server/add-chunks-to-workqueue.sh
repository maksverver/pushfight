#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
input=../input/r${phase}.bin

../count-r1 "$input" | stdbuf -oL head -100 | {
  echo 'BEGIN;'

  read line  # skip header line
  while read chunk ties losses wins total; do
    echo "$chunk $ties" >&2
    echo "INSERT INTO WorkQueue (phase, chunk, difficulty) VALUES ($phase, $chunk, $ties);"
  done

  echo 'COMMIT;'

} | sqlite3 chunks.db
