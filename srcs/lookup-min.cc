// Tool to look up a value at a specific index in a minimized merged file,
// along with best moves.
//
// Usage:
//
//  lookup-min [-d] <minimized.bin> <permutation>
//
// Where <permutation> can be in any format accepted by ParsePerm(), although
// the tool requires the permutation type to be VALID and not FINISHED.
//
// If the "-d" option is provided, detailed analysis of successors is included
// (see "Detaield output" below for more information).
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
// Detailed output:
//
// If the "-d" option is provided, then all positions with status other than W1
// or L1 are annotated with 4 more fields. The first 3 values are number of
// distinct successors that are lost, tied and won, respectively, from the
// opponent's perspective. The final field has a detailed count of different
// status. For example:
//
//  L7 b3-b2,e4-d4 +4659811013 809 50 1 W7*1,T*50,L8*1,L6*2,L3*5,L2*21,L1*780
//
// The first three fields are the same as before. 809 is the number of losing
// successors for the opponent, 50 is the number of ties, and 1 is the number of
// winning successors. The final line shows that there is 1 win-in-7, 50 ties,
// 1 loss-in-8, 2 losses-in-6, and so on.
//
// Exit status:
//
//  0 success
//  1 internal error (e.g. file not found)
//  2 client error (e.g. invalid argument)
//
// Note that the input/output format of this tool must be strictly specified
// because it is used by lookup-min-http-server.py.

#include <cassert>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "board.h"
#include "dedupe.h"
#include "macros.h"
#include "minimized-accessor.h"
#include "minimized-lookup.h"
#include "perms.h"
#include "search.h"
#include "position-value.h"

int main(int argc, char *argv[]) {
  bool detailed = false;

  // Parse options.
  {
    int j = 1;
    for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-d") == 0) {
        detailed = true;
        continue;
      }
      argv[j++] = argv[i];
    }
    for (int i = j; i < argc; ++i) argv[i] = nullptr;
    argc = j;
  }

  if (argc != 3) {
    std::cerr <<
        "Usage: lookup-min [-d] <minimized.bin> <permutation>\n";
    return 2;
  }

  const char *filename = argv[1];
  const char *perm_string = argv[2];

  MinimizedAccessor acc(filename);

  std::string error;
  auto result = LookupSuccessors(acc, perm_string, &error);
  if (!result) {
    std::cerr << error << std::endl;
    return 2;
  }

  std::vector<Perm> perms_to_lookup;
  std::vector<std::vector<Value>> succ_values;
  if (detailed) {
    // Calculate values of successors of successors.
    for (const auto &elem : *result) {
      if (elem.state.outcome == TIE) {
        if (elem.value == Value::LossIn(1)) break;
        perms_to_lookup.push_back(elem.state.perm);
      }
    }
    succ_values = LookupSuccessorValues(acc, perms_to_lookup);
  }

  size_t succ_values_index = 0;
  for (const auto &elem : *result) {
    std::cout << elem.value << ' ' << elem.moves;
    if (elem.state.outcome == TIE) {
      std::cout << ' ' << "+-"[elem.rotated] << elem.min_index;

      if (detailed && elem.value != Value::LossIn(1)) {
        // Print values of successors of successors.
        std::ostringstream details;
        int counts[3] = {0, 0, 0};
        assert(succ_values_index < succ_values.size());
        if (succ_values_index < succ_values.size()) {
          const std::vector<Value> &values = succ_values[succ_values_index++];
          for (size_t i = 0, j = 0; i < values.size(); i = j) {
            Value v = values[i];
            while (j < values.size() && values[j] == v) ++j;
            int n = j - i;
            counts[v.Sign() + 1] += n;
            if (i > 0) details << ',';
            details << v << '*' << n;
          }
        }
        for (int c : counts) std::cout << ' ' << c;
        std::cout << ' ' << details.str();
      }
    }
    std::cout << '\n';
  }
  assert(succ_values_index == succ_values.size());
  std::cout.flush();
}
