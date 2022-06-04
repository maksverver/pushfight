#!/bin/sh

# Collects the chunks from an rN phase by hardlinking files from the
# incoming/ subdirectory to the chunks/ subdirectory. Files are named
# like "chunk-r0-0000.bin".

set -e -E -o pipefail

phase=$1

if [ -z "$phase" ]; then
  echo 'Missing phase'
  exit 1
fi

sqlite3 chunks.db "SELECT phase,chunk,hex(sha256sum) FROM WorkQueue WHERE phase=$phase" | {
  IFS='|'
  while read phase chunk sha256sum
  do
    sha256sum=$(echo "$sha256sum" | tr A-Z a-z)
    src=incoming/$sha256sum
    dst=$(printf 'chunks/chunk-r%d-%04d.bin' $phase $chunk)
    if [ ! -f "$src" ]; then
      echo "$src missing! Chunk: $chunk"
      exit 1
    fi
    if [ -f "$dst" ]; then
      echo "$dst already exists. Skipping."
    elif ! ln "$src" "$dst"; then
      echo "Could not link $src to $dst!"
      exit 1
    fi
  done
}
