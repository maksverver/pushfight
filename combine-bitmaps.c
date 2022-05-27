// Merges binary input files using the binary OR operator.
//
// Note: all input files must have the same file size, or the output file
// will be truncated to the length of the first file.
//
// To use this with compressed files, use process substitution. For example:
//
//  ./combine-bitmaps <(zstdcat input1.zst) <(zstdcat inputfile2.zst) | zstd > output.zst
//

#include "macros.h"

#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUFFER_SIZE (1 << 16)  // 64 KiB

static void Combine8(uint8_t out[], uint8_t in[], size_t elems) {
  for (size_t i = 0; i < elems; ++i) {
    out[i] |= in[i];
  }
}

static void Combine64(uint64_t out[], uint64_t in[], size_t elems) {
  for (size_t i = 0; i < elems; ++i) {
    out[i] |= in[i];
  }
}

static void Combine(FILE *output, FILE **inputs, int ninput) {
  if (ninput < 1) return;
  alignas(uint64_t) uint8_t buf_in[BUFFER_SIZE];
  alignas(uint64_t) uint8_t buf_out[BUFFER_SIZE];
  size_t n;
  while ((n = fread(buf_out, sizeof *buf_out, BUFFER_SIZE, inputs[0])) != 0) {
    FOR(i, 1, ninput) {
      size_t m = fread(buf_in, sizeof *buf_in, n, inputs[i]);
      if (m != n) {
        fprintf(stderr, "Short read from input file %d!\n", i + 1);
        exit(1);
      }
      if (n % 8 == 0) {
        Combine64((uint64_t*) buf_out, (uint64_t*) buf_in, n / 8);
      } else {
        Combine8(buf_out, buf_in, n);
      }
    }
    if (fwrite(buf_out, sizeof *buf_out, n, output) != n) {
        fprintf(stderr, "Short write to output!\n");
        exit(1);
    }
  }
  if (!feof(inputs[0])) {
    fprintf(stderr, "End-of-file not reached!\n");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: combine-bitmaps <file1> <file2> .. <fileN>\n\n"
      "Note: writes combined bitmap to standard output!\n");
  }

  int nfile = argc - 1;
  FILE *files[nfile];
  REP(i, nfile) {
    files[i] = fopen(argv[i + 1], "rb");
    if (files[i] == NULL) {
      fprintf(stderr, "Could not open file %d (%s) for reading!\n", i + 1, argv[i + 1]);
      exit(1);
    }
  }

  Combine(stdout, files, nfile);

  REP(i, nfile) fclose(files[i]);
}
