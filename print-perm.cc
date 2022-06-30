#include "board.h"
#include "flags.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {

bool print_successors = false;
bool print_predecessors = false;
bool print_compact = false;
bool print_coords = false;

PrettyPerm Print(const Perm &perm) {
  return PrettyPerm{.perm=perm, .compact=print_compact, .coords=print_coords};
}

PrettyState Print(const State &state) {
  return PrettyState{.state=state, .compact=print_compact, .coords=print_coords};
}

void PrintPermId(std::ostream &os, const Perm &perm) {
  os << IndexOf(perm) << ' ';
  if (IsReachable(perm)) {
    bool rotated = false;
    int64_t min_index = MinIndexOf(perm, &rotated);
    os << "(min: " << "+-"[rotated] << min_index << ')';
  } else {
    os << "(unreachable)";
  }
  os << '\n';
}

void DumpPerm(const Perm &perm) {
  PrintPermId(std::cout, perm);
  std::cout << Print(perm) << std::endl;

  Outcome o = LOSS;
  if (print_successors) {
    std::cout << "\nSuccessors:\n\n";
  }

  GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
    if (print_successors) {
      assert((state.outcome == TIE) == IsValid(state.perm));
      std::cout << moves << '\n';
      if (state.outcome == TIE) {
        PrintPermId(std::cout, state.perm);
      } else {
        std::cout << "(game over)\n";
      }
      std::cout << Print(state) << '\n';
      if (print_compact) std::cout << '\n';
    }
    o = MaxOutcome(o, Invert(state.outcome));
    return true;
  });

  if (print_predecessors) {
    std::cout << "\nPredecessors:\n\n";
    GeneratePredecessors(perm, [](const Perm &pred) {
      PrintPermId(std::cout, pred);
      std::cout << Print(pred) << '\n';
      if (print_compact) std::cout << '\n';
    });
  }

  std::cout << "Verdict: " << (o == WIN ? "win" : o == LOSS ? "loss" : "indeterminate") << std::endl;
}

void PrintUsage() {
  std::cout << "Usage:\n" <<
    "  print-perm [options] 123   (standard permutation index)\n"
    "  print-perm [options] +456  (minimized permutation index)\n"
    "  print-perm [options] .OX.....oxY....Oox.....OX.  (compact permutation)\n\n"
    "Options:\n"
    "  --compact: print permutations without spaces\n"
    "  --coords: include coordinates in output\n"
    "  --succ: print successors\n"
    "  --pred: print predecessors" << std::endl;
}

int Count(const char *s, char target) {
  int n = 0;
  while (*s) {
    n += *s++ == target;
  }
  return n;
}

bool AllDigits(const char *s) {
  do {
    if (!isdigit(*s++)) return false;
  } while (*s);
  return true;
}

}  // namespace

int main(int argc, char *argv[]) {
  InitializePerms();

  std::string arg_compact;
  std::string arg_coords;
  std::string arg_succ;
  std::string arg_pred;

  std::map<std::string, Flag> flags = {
    {"compact", Flag::optional(arg_compact)},
    {"coords", Flag::optional(arg_coords)},
    {"succ", Flag::optional(arg_succ)},
    {"pred", Flag::optional(arg_pred)},
  };

  if (argc == 1) {
    PrintUsage();
    return 0;
  }

  if (!ParseFlags(argc, argv, flags)) {
    std::cout << "\n";
    PrintUsage();
    return 1;
  }

  if (argc < 2 || argc > 2) {
    std::cout << "Expected exactly 1 permutation argument!\n\n";
    PrintUsage();
    return 1;
  }
  const char *perm_arg = argv[1];

  print_compact = arg_compact == "true";
  print_coords = arg_coords == "true";
  print_predecessors = arg_pred == "true";
  print_successors = arg_succ == "true";

  Perm perm;
  if (AllDigits(perm_arg)) {
    // Parse as permutation index.
    int64_t index = ParseInt64(perm_arg);
    if (index < 0 || index >= total_perms) {
      std::cout << "Invalid permutation index " << index << std::endl;
      return 1;
    }
    perm = PermAtIndex(index);
  } else if ((perm_arg[0] == '+' || perm_arg[0] == '-') && AllDigits(perm_arg + 1)) {
    // Parse as minimized permutation index.
    bool rotated = perm_arg[0] == '-';
    int64_t min_index = ParseInt64(perm_arg + 1);
    if (min_index < 0 || min_index >= min_index_size) {
      std::cout << "Invalid minimized index " << min_index << std::endl;
      return 1;
    }
    perm = PermAtMinIndex(min_index, rotated);
  } else {
    // Parse as permutation string.
    if (strlen(perm_arg) != 26) {
      std::cout << "Invalid length! Expected 26." << std::endl;
      return 1;
    }
    if (Count(perm_arg, 'o') != 2 ||
        Count(perm_arg, 'O') != 3 ||
        Count(perm_arg, 'x') != 2 ||
        Count(perm_arg, 'X') != 2 ||
        Count(perm_arg, 'Y') != 1) {
      std::cout << "Invalid character counts." << std::endl;
      return 1;
    }
    REP(i, 26) {
      char ch = perm_arg[i];
      perm[i] =
          ch == 'o' ? WHITE_MOVER :
          ch == 'O' ? WHITE_PUSHER :
          ch == 'x' ? BLACK_MOVER :
          ch == 'X' ? BLACK_PUSHER :
          ch == 'Y' ? BLACK_ANCHOR : EMPTY;
    }
  }
  DumpPerm(perm);
  return 0;
}
