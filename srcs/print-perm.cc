#include "board.h"
#include "flags.h"
#include "parse-perm.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <cctype>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>

namespace {

bool print_compact = false;
bool print_coords = false;
bool print_successors = false;
bool print_predecessors = false;
bool print_unique = false;

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

void DumpPerm(PermType type, const Perm &perm) {
  assert(type != PermType::INVALID);

  if (type == PermType::IN_PROGRESS) {
    PrintPermId(std::cout, perm);
  }
  std::cout << Print(perm) << std::endl;

  Outcome o;

  if (type == PermType::FINISHED) {
    o = GetOutcome(perm);
    if (print_successors) {
      std::cout << "Cannot print successors because this position is finished.\n";
    }
  } else {
    assert(type == PermType::STARTED || type == PermType::IN_PROGRESS);

    if (print_successors) {
      std::cout << "\nSuccessors:\n\n";
    }

    o = LOSS;  // will be updated below
    std::map<Perm, Outcome> seen;
    GenerateSuccessors(perm, [&](const Moves &moves, const State &state) {
      assert(ValidatePerm(state.perm) == (state.outcome == TIE ? PermType::IN_PROGRESS : PermType::FINISHED));
      auto it = seen.find(state.perm);
      bool dupe = false;
      if (it == seen.end()) {
        seen[state.perm] = state.outcome;
      } else {
        assert(it->second == state.outcome);
        dupe = true;
      }
      if (print_successors && (!print_unique || !dupe)) {
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
  }

  if (print_predecessors) {
    if (type == PermType::STARTED) {
      std::cout << "Cannot print predecessors because this position is just started.\n";
    } else if (type == PermType::FINISHED) {
      // In theory, it's' possible to generate predecessor for finished
      // positions too, since we can figure out which piece was just pushed off
      // the board. However, this isn't currently implemented.
      std::cout << "Cannot print predecessors because this position is finished.\n";
    } else {
      std::cout << "\nPredecessors:\n\n";
      std::set<Perm> seen;
      GeneratePredecessors(perm, [&](const Perm &pred) {
        if (!print_unique || seen.insert(pred).second) {
          PrintPermId(std::cout, pred);
          std::cout << Print(pred) << '\n';
          if (print_compact) std::cout << '\n';
        }
      });
    }
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
    "  --pred: print predecessors\n"
    "  --uniq: only show one possible turn leading to each unique state"
    << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
  std::string arg_compact;
  std::string arg_coords;
  std::string arg_succ;
  std::string arg_pred;
  std::string arg_uniq;

  std::map<std::string, Flag> flags = {
    {"compact", Flag::optional(arg_compact)},
    {"coords", Flag::optional(arg_coords)},
    {"succ", Flag::optional(arg_succ)},
    {"pred", Flag::optional(arg_pred)},
    {"uniq", Flag::optional(arg_uniq)},
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
  print_unique = arg_uniq == "true";

  std::string error;
  std::optional<Perm> perm = ParsePerm(perm_arg, &error);
  if (!perm) {
    std::cout << "Could not parse \"" << perm_arg << "\" as a permutation: "
        << error << std::endl;
    return 1;
  }

  PermType type = ValidatePerm(*perm);
  if (type == PermType::INVALID) {
    std::cout << "Invalid permutation: " << perm_arg << std::endl;
    return 1;
  }

  DumpPerm(type, *perm);
  return 0;
}
