// Tool to look up a value at a specific index in a minimized merged file,
// along with best moves.

#include <cassert>
#include <iostream>
#include <iomanip>
#include <optional>
#include <string>

#include "accessors.h"
#include "board.h"
#include "macros.h"
#include "perms.h"
#include "parse-perm.h"
#include "search.h"

namespace {

struct Value {
  static Value LossIn(int moves) { assert(moves >= 0); return Value(moves*2 + 1); }
  static Value WinIn(int moves) { assert(moves > 0); return Value(moves*2); }

  Value() : byte(0) {}

  explicit Value(uint8_t byte) : byte(byte) {}

  // Returns +1 if the position is won, -1 if lost, or 0 if tied.
  int Sign() const {
    return byte == 0 ? 0 : (byte&1) ? -1 : +1;
  }

  // Returns the number of moves left (0 if tied, otherwise a nonnegative value)
  int Magnitude() const {
    return byte >> 1;
  }

  uint8_t byte;
};

Value operator-(const Value &v) {
  return Value(v.byte == 0 ? 0 : v.byte + 1);
}

bool operator==(const Value &a, const Value &b) {
  return a.byte == b.byte;
}

// Evaluates whether value `a` comes before value `b`. The sort order is
// descending by default (i.e., the best moves come first.)
//
// Winning is better than tying which is better than losing.
//
// If both positions are winning, the one which takes fewer moves is better.
// If both positions are losing, the one which takes more moves is better.
bool operator<(const Value &a, const Value &b) {
  int x = a.Sign();
  int y = b.Sign();
  return x != y ? x > y : x > 0 ? a.Magnitude() < b.Magnitude() : x < 0 ? a.Magnitude() > b.Magnitude() : 0;
}

std::ostream &operator<<(std::ostream &os, const Value &v) {
  if (v.Sign() == 0) return os << "T";
  return os << "WL"[v.Sign() < 0] << v.Magnitude();
}

}  // namespace

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout <<
        "Usage: lookup-min <minimized.bin> <permutation>\n";
    return 0;
  }

  const char *filename = argv[1];
  const char *perm_string = argv[2];

  InitializePerms();

  // Parse permutation argument.
  Perm perm;
  {
    std::string error;
    std::optional<Perm> res = ParsePerm(perm_string, &error);
    if (!res) {
      std::cout << "Could not parse permutation string \"" << perm_string << "\": "
          << error << std::endl;
      return 1;
    }
    perm = std::move(*res);
    if (ValidatePerm(perm) != PermType::IN_PROGRESS) {
      std::cout << "Permutation does not represent an in-progress position!" << std::endl;
      return 1;
    }
    if (!IsReachable(perm)) {
      std::cout << "Position is unreachable!" << std::endl;
      return 1;
    }
  }

  MappedFile<uint8_t, min_index_size> acc(filename);

  const int64_t min_index = MinIndexOf(perm);
  const Value stored_value = Value(acc[min_index]);

  std::vector<std::pair<Moves, State>> successors = GenerateAllSuccessors(perm);
  Deduplicate(successors);

  std::vector<std::pair<Value, std::pair<Moves, State>>> evaluated_successors;
  for (const std::pair<Moves, State> &elem : successors) {
    const Outcome o = elem.second.outcome;
    const Perm &p = elem.second.perm;
    Value value;
    if (o == LOSS) {
      assert(IsFinished(p));
      // If the successor is losing for the next player, the moves were
      // winning for the last player.
      value = Value::WinIn(1);
    } else if (o == WIN) {
      assert(IsFinished(p));
      // Symmetric to above. Currently GenerateAllSuccessors() does not return
      // losing moves, so this code never executes.
      value = Value::LossIn(1);
    } else {
      assert(o == TIE);
      assert(IsInProgress(p));
      assert(IsReachable(p));
      int64_t min_index = MinIndexOf(elem.second.perm);
      value = -Value(acc[min_index]);
    }
    evaluated_successors.push_back({value, elem});
  };

  // Sort by descending value, so that the best moves come first.
  std::sort(evaluated_successors.begin(), evaluated_successors.end(),
    [](
        const std::pair<Value, std::pair<Moves, State>> &a,
        const std::pair<Value, std::pair<Moves, State>> &b) {
      return a.first < b.first;
    });

  if (evaluated_successors.empty()) {
    assert(stored_value == Value::LossIn(0));
  } else {
    for (const auto &elem : evaluated_successors) {
      const Value &value = elem.first;
      const Moves &moves = elem.second.first;
      const State &state = elem.second.second;
      std::cout << value << ' ' << moves;
      if (state.outcome == TIE) {
        bool rotated = false;
        int64_t min_index = MinIndexOf(state.perm, &rotated);
        std::cout << ' ' << "+-"[rotated] << min_index;
      }
      std::cout << '\n';
    }
    std::cout.flush();
    assert(stored_value == evaluated_successors.front().first);
  }
}
