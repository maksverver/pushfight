#include "board.h"
#include "flags.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace {

bool print_successors = false;
bool print_predecessors = false;
bool print_compact = false;

void DumpPerm(const Perm &perm) {
  std::cout << IndexOf(perm) << '\n'
      << PrettyPerm{.perm=perm, .compact=print_compact} << std::endl;

  std::cout << "This position is " << (IsReachable(perm) ? "likely" : "NOT") << " reachable.\n";

  Outcome o = LOSS;
  if (print_successors) {
    std::cout << "\nSuccessors:\n\n";
  }

  GenerateSuccessors(perm, [&o](const Moves &moves, const State &state) {
    if (print_successors) {
      std::cout << moves << '\n';
      std::cout << IndexOf(state.perm) << '\n';
      std::cout << PrettyState{.state=state, .compact=print_compact} << '\n';
      if (print_compact) std::cout << '\n';
    }
    o = MaxOutcome(o, Invert(state.outcome));
    return true;
  });

  if (print_predecessors) {
    std::cout << "\nPredecessors:\n\n";
    GeneratePredecessors(perm, [](const Perm &pred) {
      std::cout << IndexOf(pred) << '\n';
      std::cout << PrettyPerm{.perm=pred, .compact=print_compact} << "\n";
      if (print_compact) std::cout << '\n';
    });
  }

  std::cout << "Verdict: " << (o == WIN ? "win" : o == LOSS ? "loss" : "indeterminate") << std::endl;
}

int count(const std::string &s, char target) {
  int n = 0;
  for (char ch : s) if (ch == target) ++n;
  return n;
}

void PrintUsage() {
  std::cout << "Usage:\n" <<
    "  print-perm [options] --index=123\n"
    "  print-perm [options] --perm=.OX.....oxY....Oox.....OX.\n\n"
    "Options:\n"
    "  --compact: print permutations without spaces\n"
    "  --succ: print successors\n"
    "  --pred: print predecessors" << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  InitializePerms();

  std::string arg_index;
  std::string arg_perm;
  std::string arg_compact;
  std::string arg_succ;
  std::string arg_pred;

  std::map<std::string, Flag> flags = {
    {"index", Flag::optional(arg_index)},
    {"perm", Flag::optional(arg_perm)},
    {"compact", Flag::optional(arg_compact)},
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

  if (argc > 1) {
    std::cout << "Too many arguments!\n\n";
    PrintUsage();
    return 1;
  }

  if (arg_index.empty() == arg_perm.empty()) {
    std::cout << "Exactly one of --index or --perm must be provided.\n";
    return 1;
  }

  print_compact = arg_compact == "true";
  print_predecessors = arg_pred == "true";
  print_successors = arg_succ == "true";

  if (!arg_index.empty()) {
    int64_t index = ParseInt64(arg_index.c_str());
    if (index < 0 || index >= total_perms) {
      std::cout << "Invalid permutation index " << index << std::endl;
    }
    Perm perm = PermAtIndex(index);
    DumpPerm(perm);
  } else if (!arg_perm.empty()) {
    if (arg_perm.size() != 26) {
      std::cout << "Invalid length! Expected 26." << std::endl;
    } else if (
        count(arg_perm, 'o') != 2 ||
        count(arg_perm, 'O') != 3 ||
        count(arg_perm, 'x') != 2 ||
        count(arg_perm, 'X') != 2 ||
        count(arg_perm, 'Y') != 1) {
      std::cout << "Invalid character counts." << std::endl;
    } else {
      Perm perm;
      REP(i, 26) {
        char ch = arg_perm[i];
        perm[i] =
            ch == 'o' ? WHITE_MOVER :
            ch == 'O' ? WHITE_PUSHER :
            ch == 'x' ? BLACK_MOVER :
            ch == 'X' ? BLACK_PUSHER :
            ch == 'Y' ? BLACK_ANCHOR : EMPTY;
      }
      DumpPerm(perm);
    }
  }
}
