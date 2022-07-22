#include "parse-perm.h"

#include "board.h"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {

/*
int Count(std::string_view s, char target) {
  int n = 0;
  for (char ch : s) n += ch == target;
  return n;
}
*/

bool AllDigits(std::string_view s) {
  for (char ch : s) {
    if (!isdigit(ch)) return false;
  }
  return true;
}

}  // namespace

std::optional<Perm> ParsePerm(std::string_view s, std::string *error) {
  if (s.empty()) {
    if (error) *error = "String is empty";
    return {};
  }

  if (AllDigits(s)) {
    // Parse as permutation index.
    std::istringstream iss = std::istringstream(std::string(s));
    int64_t index = -1;
    iss >> index;
    if (index < 0 || index >= total_perms) {
      if (error) *error = "Permutation index out of range";
      return {};
    }

    return PermAtIndex(index);
  }

  if ((s[0] == '+' || s[0] == '-') && AllDigits(s.substr(1))) {
    // Parse as minimized permutation index.
    bool rotated = s[0] == '-';
    std::istringstream iss = std::istringstream(std::string(s.substr(1)));
    int64_t min_index = -1;
    iss >> min_index;
    if (min_index < 0 || min_index >= min_index_size) {
      if (error) *error = "Minimized index out of range";
      return {};
    }

    return PermAtMinIndex(min_index, rotated);
  }

  // Parse as compact permutation string.
  static_assert(L == 26);
  if (s.size() != 26) {
    if (error) *error = "Invalid length (expected 26)";
    return {};
  }

  Perm perm;
  for (int i = 0; i < 26; ++i) {
    char ch = s[i];
    perm[i] =
        ch == 'o' ? WHITE_MOVER :
        ch == 'O' ? WHITE_PUSHER :
        ch == 'x' ? BLACK_MOVER :
        ch == 'X' ? BLACK_PUSHER :
        ch == 'Y' ? BLACK_ANCHOR : EMPTY;
  }
  return perm;
}
