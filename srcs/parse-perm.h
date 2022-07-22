#ifndef PARSE_PERM_H_INCLUDED
#define PARSE_PERM_H_INCLUDED

#include "perms.h"

#include <optional>
#include <string>
#include <string_view>

// Parses a string into a Push Fight permutation.
//
// The following formats are accepted:
//
//  - permutation index (e.g. "194131625612"), see IndexOf() in perms.h,
//    which denotes an in-progress permutation.
//
//  - minimized index (e.g. "-8401495373"), see MinIndexOf() in perms.h
//    where the leading +/- sign indicates whether the pieces are oriented
//    normally (+) or rotated by 180 degrees (-), which denotes an
//    in-progress permutation.
//
//  - compact permutation string of 26 characters,
//    (e.g. ".OX.....oxX.....Oox...OY.."), which can denote any type of
//    permutation, not necessarily a valid one!
//
// That means ParsePerm("194131625612"), ParsePerm("-8401495373") and
// ParsePerm(".OX.....oxX.....Oox...OY..") should all return the same
// result.
//
// If the string `s` can be parsed, the decoded permutation is returned. If it
// cannot be parsed, an empty optional is returned, and if `error` is non-null,
// an error message will be assigned to *error.
//
// The returned permutation may not be valid; the caller should use
// ValidatePerm() or a related function to validate it.
std::optional<Perm> ParsePerm(std::string_view s, std::string *error = nullptr);

#endif  // ndef PARSE_PERM_H_INCLUDED
