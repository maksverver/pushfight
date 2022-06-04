// Routines for encoding/decoding (multi)sets of integers in Elias-Fano coding.
//
// Note: if the input is a set of distinct integers (rather than a multiset) and
// the size of the set is close to the range of values (e.g. a set of n elements
// between 0 and m exclusive, where m/2 < n <= m), then it will be more
// efficient to encode the complement of the set!

#ifndef EFCODE_H_INCLUDED
#define EFCODE_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

#include "bytes.h"
#include "byte_span.h"

// Encode a sorted list of nonnegative integers (deplicates are allowed) into
// a compact byte array using Elias-Fano encoding.
//
// This encoding packs N integers between 0 and M (exclusive) into
// N * (2 + ceil(log(M/N, 2))) bits, which is close to optimal.
//
// k specifies the number of tail bits to use. Normally it should be left at -1
// which means it will be determined automatically by calling EFTailBits().
bytes_t EncodeEF(const std::vector<int64_t> sorted_ints, int k = -1);

// Decodes a byte array produced by EncodeEF() above. The result is an optional
// containing a vector of nondecreasing nonnegative integers, or an empty
// optional if the input byte array was not encoded correctly.
//
// TODO later: support returning the end index, so this can be used to parse
// just a prefix of a binary string?
std::optional<std::vector<int64_t>> DecodeEF(byte_span_t bytes);

// Returns the number of bits that are encoded literally in the Elias-Fano
// encoding of `n` integers between 0 and `m` (inclusive!)
//
// n must be positive.
// m must be nonnegative.
//
// This is used internally by EncodeEF() and also in tests.
int EFTailBits(int64_t n, int64_t m);

#endif  // ndef EFCODE_H_INCLUDED
