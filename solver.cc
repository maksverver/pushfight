#include "macros.h"
#include "perms.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace {

// Number of threads to use for calculations. 0 to disable multithreading.
constexpr int num_threads = 8;

constexpr int H = 4;
constexpr int W = 8;

constexpr std::array<std::array<int, W>, H> BOARD_INDEX = {
  std::array<int, 8>{ -1, -1,  0,  1,  2,  3,  4, -1 },
  std::array<int, 8>{  5,  6,  7,  8,  9, 10, 11, 12 },
  std::array<int, 8>{ 13, 14, 15, 16, 17, 18, 19, 20 },
  std::array<int, 8>{ -1, 21, 22, 23, 24, 25, -1, -1 },
};

constexpr int DANGER_POSITIONS[] = {
  0, 4, 5, 6, 12, 13, 19, 20, 21, 25
};

constexpr std::array<int, L> FIELD_ROW = {
          0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,
  2,  2,  2,  2,  2,  2,  2,  2,
      3,  3,  3,  3, 3,
};

constexpr std::array<int, L> FIELD_COL = {
          2,  3,  4,  5,  6,
  0,  1,  2,  3,  4,  5,  6,  7,
  0,  1,  2,  3,  4,  5,  6,  7,
      1,  2,  3,  4,  5
};

const int DR[4] = { -1,  0,  0, 1 };
const int DC[4] = {  0, -1, +1, 0 };

const std::string chars = ".oOxXY";

constexpr int EMPTY        = 0;
constexpr int WHITE_MOVER  = 1;
constexpr int WHITE_PUSHER = 2;
constexpr int BLACK_MOVER  = 3;
constexpr int BLACK_PUSHER = 4;
constexpr int BLACK_ANCHOR = 5;

const char INVERSE_PIECE[6] = {
  EMPTY,
  BLACK_MOVER,
  BLACK_PUSHER,
  WHITE_MOVER,
  WHITE_PUSHER,
  WHITE_PUSHER,
};

constexpr Perm initial_state = {
        0, 2, 4, 0, 0,
  0, 0, 0, 1, 3, 5, 0, 0,
  0, 0, 2, 1, 3, 0, 0, 0,
     0, 0, 2, 4, 0,
};

enum Outcome {
  TIE  = 0,
  LOSS = 1,
  WIN  = 2,
};

Outcome MaxOutcome(Outcome a, Outcome b) {
  if (a == WIN || b == WIN) return WIN;
  if (a == TIE || b == TIE) return TIE;
  return LOSS;
}

const Outcome INVERSE_OUTCOME[3] = { TIE, WIN, LOSS };

struct State {
  Perm perm;
  Outcome outcome;
};

struct Moves {
  // Number of moves followed by a final push. Between 1 and 3 (inclusive).
  int size;

  // The actual moves. Each pair is a field index: from, to. Where `from` is
  // the index of a field occupied by the current player, and `to` is a field
  // that is empty (for move moves) or an adjacent occupied field
  // (for push moves).
  std::array<std::pair<int, int>, 3> moves;
};

int getBoardIndex(int r, int c) {
  return r >= 0 && r < H && c >= 0 && c < W ? BOARD_INDEX[r][c] : -1;
}

int getNeighbourIndex(int i, int d) {
  return getBoardIndex(FIELD_ROW[i] + DR[d], FIELD_COL[i] + DC[d]);
}


std::string FieldToId(int i) {
  std::string s(2, '\0');
  s[0] = 'a' + FIELD_COL[i];
  s[1] = '4' - FIELD_ROW[i];
  return s;
}

std::ostream &Dump(const Perm &p, std::ostream &os) {
  REP(r, H) {
    REP(c, W) {
      int i = BOARD_INDEX
    [r][c];
      os << (i >= 0 && i < p.size() ? (p[i] >= 0 && p[i] < chars.size() ? chars[p[i]] : '?') : ' ');
    }
    os << '\n';
  }
  return os;
}

std::ostream &Dump(const State &s, std::ostream &os) {
  Outcome o = s.outcome;
  return Dump(s.perm, os) <<
      "Outcome: " << (o == WIN ? "win" : o == LOSS ? "loss" : "indeterminate") << '\n';
}

std::ostream &Dump(const Moves &moves, std::ostream &os) {
  REP(i, moves.size) {
    if (i > 0) os << ',';
    os << FieldToId(moves.moves[i].first) << '-' << FieldToId(moves.moves[i].second);
  }
  return os;
}

bool IsValidPush(const Perm &perm, int i, int d) {
  const int dr = DR[d];
  const int dc = DC[d];
  int r = FIELD_ROW[i] + dr;
  int c = FIELD_COL[i] + dc;
  i = getBoardIndex(r, c);
  if (i < 0 || perm[i] == EMPTY) {
    // Must push at least one piece.
    return false;
  }
  while (i != -1 && perm[i] != EMPTY) {
    if (perm[i] == BLACK_ANCHOR) {
      // Cannot push anchored piece.
      return false;
    }
    r += dr;
    c += dc;
    if (r < 0 || r >= H) {
      // Canot push pieces past the anchor at the top/bottom of the board.
      return false;
    }
    i = getBoardIndex(r, c);
  }
  return true;
}

// Executes a push move, flipping the pieces so the black becomes white and vice versa, and
// returns whether the move returns in an instant win or loss for the opponent (i.e., if
// white pushes a black piece off the board, the result contains only 4 white pieces and the
// result is LOSS to indicate the next player loses.)
Outcome ExecutePush(Perm &perm, int i, int d) {
  // Flip position, replace anchor.
  REP(j, L) perm[j] = INVERSE_PIECE[int{perm[j]}];
  perm[i] = BLACK_ANCHOR;

  // Move pushed pieces.
  int r = FIELD_ROW[i];
  int c = FIELD_COL[i];
  int dr = DR[d];
  int dc = DC[d];
  int f = EMPTY;
  for (;;) {
    const int j = getBoardIndex(r, c);
    if (j == -1) {
      // Piece pushed over the edge of the board.
      assert(f != EMPTY);
      return f == WHITE_MOVER || f == WHITE_PUSHER ? LOSS : WIN;
    }
    int g = perm[j];
    perm[j] = f;
    f = g;
    if (f == EMPTY) {
      // No more pieces to move.
      return TIE;
    }
    r += dr;
    c += dc;
  }
}

// Possible optimization: deduplicate states after move(s), just before push?
// That would make deduplicating after the fact unnecessary.

bool GenerateSuccessors(
    Perm &perm,
    Moves &moves,
    std::function<bool(const Moves&, const State&)> &callback) {
  // Generate push moves.
  REP(i, L) if (perm[i] == WHITE_PUSHER) REP(d, 4) if (IsValidPush(perm, i, d)) {
    moves.moves[moves.size] = {i, getNeighbourIndex(i, d)};
    ++moves.size;

    // Possible optimization: undo push instead of copying the permutation?
    State state = {.perm = perm, .outcome = TIE};
    state.outcome = ExecutePush(state.perm, i, d);

    if (!callback(moves, state)) return false;

    --moves.size;
  }

  if (moves.size < 2) {
    // Generate moves.
    std::vector<int> todo;
    REP(i0, L) if (perm[i0] == WHITE_MOVER || perm[i0] == WHITE_PUSHER) {
      // Optimization: don't move the same piece twice. There is never any reason for it.
      if (moves.size > 0 && moves.moves[moves.size - 1].second == i0) continue;

      todo.assign(1, i0);
      uint32_t visited = uint32_t{1} << i0;
      for (int j = 0; j < todo.size(); ++j) {
        const int i1 = todo[j];
        REP(d, 4) {
          if (const int i2 = getNeighbourIndex(i1, d); i2 >= 0 && perm[i2] == EMPTY && (visited & (uint32_t{1} << i2)) == 0) {
            visited |= uint32_t{1} << i2;
            todo.push_back(i2);

            moves.moves[moves.size] = {i0, i2};
            ++moves.size;

            std::swap(perm[i0], perm[i2]);
            if (!GenerateSuccessors(perm, moves, callback)) return false;
            std::swap(perm[i0], perm[i2]);

            --moves.size;
          }
        }
      }
    }
  }

  return true;
}

// Enumerates the successors of `perm`.
bool GenerateSuccessors(
    const Perm &perm,
    std::function<bool(const Moves&, const State&)> callback) {
  Perm mutable_perm = perm;
  Moves moves = {.size = 0, .moves={}};
  return GenerateSuccessors(mutable_perm, moves, callback);
}

void Deduplicate(std::vector<std::pair<Moves, State>> &successors) {
  auto lt = [](const std::pair<Moves, State> &a, const std::pair<Moves, State> &b) {
    return a.second.perm < b.second.perm;
  };
  auto eq = [](const std::pair<Moves, State> &a, const std::pair<Moves, State> &b) {
    return a.second.perm == b.second.perm;
  };
  std::sort(successors.begin(), successors.end(), lt);
  std::vector<std::pair<Moves, State>>::iterator it = std::unique(successors.begin(), successors.end(), eq);
  successors.erase(it, successors.end());
}

bool IsBlackInDanger(const Perm &p) {
  for (int i : DANGER_POSITIONS) {
    if (p[i] == BLACK_MOVER || p[i] == BLACK_PUSHER) return true;
  }
  return false;
}

constexpr int chunk_size = 54054000;
constexpr int num_chunks = 7429;
static_assert(int64_t{chunk_size} * int64_t{num_chunks} == total_perms);

Outcome CalculateR0(const Perm &perm) {
  // Optimization: for the first pass, only check successors if there is a
  // pushable black piece on an edge spot. Otherwise, we cannot win this turn.
  // Note: this means we cannot detect losses due to being unable to move.
  // My hypothesis is that there are no such positions, but if there are, the
  // we should interpret TIE as "not a WIN (possibly a loss)".
  if (!IsBlackInDanger(perm)) return TIE;

  Outcome o = TIE;
  int n = 0;
  bool aborted = GenerateSuccessors(perm, [&o, &n](const Moves &moves, const State &state) {
    (void) moves;  // unused
    ++n;
    if (state.outcome == LOSS) {
      o = WIN;
      return false;
    }
    return true;
  });
  if (n == 0) {
    Dump(perm, std::cout);
  }
  assert(n > 0);  // hypothesis: there is always some valid move
  return o;
}

void ComputeChunkThread(int64_t offset, int size, Outcome *outcomes) {
  Perm perm = PermAtIndex(offset);
  REP(i, size) {
    *outcomes++ = CalculateR0(perm);
    std::next_permutation(perm.begin(), perm.end());
  }
}

std::vector<Outcome> ComputeChunk(int chunk) {
  std::vector<uint8_t> bits(chunk_size / 8);
  int64_t start_index = int64_t{chunk} * int64_t{chunk_size};
  std::vector<Outcome> outcomes(chunk_size, TIE);
  if (num_threads == 0) {
    // Single-threaded computation.
    ComputeChunkThread(start_index, chunk_size, outcomes.data());
  } else {
    assert(chunk_size % num_threads == 0);
    // Multi-threaded computation.
    std::vector<std::thread> threads;
    const int thread_size = chunk_size / num_threads;
    REP(i, num_threads) {
      threads.emplace_back(ComputeChunkThread, start_index + thread_size * i, thread_size,
          outcomes.data() + thread_size * i);
    }
    REP(i, num_threads) threads[i].join();
  }
  return outcomes;
}

std::string FileName(int chunk) {
  std::ostringstream oss;
  oss << "chunk-r0-" << std::setfill('0') << std::setw(4) << chunk << ".bin";
  return oss.str();
}

void ProcessChunk(int chunk) {
  std::vector<Outcome> outcomes = ComputeChunk(chunk);
  assert(outcomes.size() % 8 == 0);
  std::vector<uint8_t> bytes(outcomes.size() / 8, uint8_t{0});
  REP(i, outcomes.size()) {
    assert(outcomes[i] == TIE || outcomes[i] == WIN);
    if (outcomes[i] == WIN) bytes[i / 8] |= 1 << (i % 8);
  }
  std::ofstream os(FileName(chunk), std::fstream::binary);
  os.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
}

}  // namespace

int main() {
  InitializePerms();
  REP(chunk, num_chunks) {
    if (std::filesystem::exists(FileName(chunk))) {
      std::cerr << "Chunk " << chunk << " already exists. Skipping..." << std::endl;
      continue;
    }
    std::cerr << "Chunk " << chunk << " calculating... " << std::flush;
    auto start_time = std::chrono::system_clock::now();
    ProcessChunk(chunk);
    std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start_time;
    std::cerr << "Done in " << elapsed_seconds.count() / 60 << " minutes." << std::endl;
  }
}
