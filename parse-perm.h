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
//  - permutation index (e.g. "194131625612"), see IndexOf() in perms.h
//
//  - minimized index (e.g. "-8401495373"), see MinIndexOf() in perms.h
//    where the leading +/- sign indicates whether the pieces are oriented
//    normally (+) or rotated by 180 degrees (-)
//
//  - compact permutation string of 26 characters
//    (e.g. ".OX.....oxX.....Oox...OY..")
//
// That means ParsePerm("194131625612"), ParsePerm("-8401495373") and
// ParsePerm(".OX.....oxX.....Oox...OY..") should all return the same
// result.
//
// If the string `s` can be parsed, the decoded permutation is returned. If it
// cannot be parsed, an empty optional is returned, and if `error` is non-null,
// an error message will be assigned to *error.
//
// Currently this function always returns a valid permutation (see IsValid() in
// perms.h). In the future it may be extended to also parse starting positions
// (i.e., without an anchor on any piece).
std::optional<Perm> ParsePerm(std::string_view s, std::string *error = nullptr);

#endif  // ndef PARSE_PERM_H_INCLUDED
