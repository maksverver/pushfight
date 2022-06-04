#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
counts_r2=../metadata/r$(expr "$phase" - 2)-counts.txt
counts_r1=../metadata/r$(expr "$phase" - 1)-counts.txt

for file in "$counts_r2" "$counts_r1"; do
  if ! test -f "$file"; then
      echo "Missing input file: $file"
      exit 1
  fi
done

join "$counts_r2" "$counts_r1" | {
  echo 'BEGIN;'

  read line  # skip header line
  while read chunk ties2 losses2 wins2 total2 ties1 losses1 wins1 total1 ; do
    if [ "$chunk" -ge 0 ] && [ "$losses2" -ne "$losses1" ]; then
        difficulty=$(expr "$losses1" - "$losses2")
        echo "$chunk $difficulty" >&2
        echo "INSERT INTO WorkQueue (phase, chunk, difficulty) VALUES ($phase, $chunk, $difficulty);"
    fi
  done

  echo 'COMMIT;'

} | sqlite3 chunks.db
