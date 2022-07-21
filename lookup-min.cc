// Tool to look up a value at a specific index in a minimized merged file,
// along with best moves.
//
// Usage:
//
//  lookup-min <minimized.bin> <permutation>
//
// Where <permutation> can be in any format accepted by ParsePerm(), although
// the tool requires the permutation type to be VALID and not FINISHED.
//
// Output format:
//
// Multiple lines, one per successor (excluding immediately losing successors,
// i.e. those where a player pushes their own piece off the board). Each line
// contains up to three fields separated with spaces. Example:
//
//  W1 c3-c4,e3-b2,b2-b3
//  L1 e3-f3,c3-e3,e2-e3 +55456658050
//
// The first field indicates the status of the position. T for tie, W7 for
// win in 7, L7 for loss in 7, etc. These numbers count the number of turns
// of the player next to move, so win-in-2 means three more turns will be
// played: p1, p2, p1 (winning), while loss-in-2 means four more turns:
// p1, p2, p1, p2 (winning).
//
// The second field is the optimal set of moves for the turn. Note that there
// may be multiple sequences of moves in a turn that lead to the same successor
// position; only one such sequence is shown per successor.
//
// The third field is the minimized index of the successor position. This is
// present only if the successor is not finished, i.e., it is missing when the
// status is W1.
//
// The lines are ordered from best to worst (i.e., winning moves first,
// then ties, than losing moves, with winning moves ordered with lowest
// number of remaining moves first, and losing moves ordered with highest
// number of remaining moves first), e.g. W1, W2, W3, T, T, L3, L2, L1.
//
// Exit status:
//
//  0 success
//  1 internal error (e.g. file not found)
//  2 client error (e.g. invalid argument)
//
// Note that the interface of this tool must be strictly specified because it
// is used by lookup-min-http-server.py.

#include <cassert>
#include <iostream>
#include <iomanip>
#include <optional>
#include <string>

#include "accessors.h"
#include "board.h"
#include "macros.h"
#include "perms.h"
#include "position-value.h"
#include "parse-perm.h"
#include "search.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr <<
        "Usage: lookup-min <minimized.bin> <permutation>\n";
    return 2;
  }

  const char *filename = argv[1];
  const char *perm_string = argv[2];

  InitializePerms();

  // Parse permutation argument.
  Perm perm;
  PermType type;
  {
    std::string error;
    std::optional<Perm> res = ParsePerm(perm_string, &error);
    if (!res) {
      std::cerr << "Could not parse permutation string \"" << perm_string << "\": "
          << error << std::endl;
      return 2;
    }
    perm = std::move(*res);
    type = ValidatePerm(perm);
    if (type == PermType::INVALID) {
      std::cerr << "Permutation is invalid!\n";
      return 2;
    }
    if (type == PermType::FINISHED) {
      std::cerr << "Permutation represents a finished position!\n";
      return 2;
    }
    assert(type == PermType::STARTED || type == PermType::IN_PROGRESS);
  }

  MappedFile<uint8_t, min_index_size> acc(filename);

  const int64_t min_index =
      type == PermType::IN_PROGRESS && IsReachable(perm) ? MinIndexOf(perm) : -1;
  const Value stored_value =
      min_index >= 0 ? Value(acc[min_index]) : Value::Tie();

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
    assert(min_index == -1 || stored_value == evaluated_successors.front().first);
  }
}
