#!/bin/sh

set -e -E -o pipefail

if [ $# -ne 1 ]; then
  echo "Usage: $0 <phase>"
  exit 1
fi

phase=$1
prev_phase=$((phase - 2))

for file in "../metadata/chunk-r${prev_phase}.sha256sum" "../input/r${prev_phase}-new.bin"; do
  if [ ! -f "$file" ]; then
    echo "Missing input file: $file"
    ./encode-input-file.py "$file" input/"$(basename "$file")"
  fi
done

./add-solve2-chunks-to-workqueue.sh "$phase"
