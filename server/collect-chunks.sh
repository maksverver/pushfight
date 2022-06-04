#!/bin/sh

# Collects the chunks from an rN phase by hardlinking files from the
# incoming/ subdirectory to the chunks/ subdirectory. Files are named
# like "chunk-r0-0000.bin".
#
# Writes the file SHA256 checksums to standard output (in the same format
# as sha256sum) which should be redirected to a file (or /dev/null).
#
# Example:
#
#  ./collect-chunks.sh 5 > chunk-r5.sha25sum


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
    filename=$(printf 'chunk-r%d-%04d.bin' "$phase" "$chunk")
    src=incoming/$sha256sum
    dst=chunks/$filename
    if [ ! -f "$src" ]; then
      echo "$src missing! Chunk: $chunk" >&2
      exit 1
    fi
    if [ -f "$dst" ]; then
      echo "$dst already exists. Skipping." >&2
    elif ! ln "$src" "$dst"; then
      echo "Could not link $src to $dst!"
      exit 1
    fi
    echo "$sha256sum  $filename"
  done
}
