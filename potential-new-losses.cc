// Calculates potential newly losing permutations from the newly winning
// permutations of a previous phase.
//
// Outputs a list of Elias-Fano encoded integers per chunk. Each list contains
// the permutation indices of potential new losses, which are defined as the
// predecessors of the newly winning positions from the last phase that are
// still marked as indeterminate.
//
// The output of this tool is used by solve3 to calculate true new losses
// (by examining all the successors of each potentially losing permutation).
//
// Note that this tool is only efficient in later phases, when the number of
// newly winning and potentially losing positions is relatively small.
//

#include "accessors.h"
#include "board.h"
#include "bytes.h"
#include "chunks.h"
#include "efcodec.h"
#include "flags.h"
#include "input-generation.h"
#include "input-verification.h"
#include "macros.h"
#include "parse-int.h"
#include "perms.h"
#include "search.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

const std::string PhaseInputFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << ".bin";
  return oss.str();
}

const std::string PhaseDiffFilename(int phase) {
  std::ostringstream oss;
  oss << "input/r" << phase << "-new.bin";
  return oss.str();
}

void PrintUsage() {
  std::cerr << "Usage: potential-new-losses --phase=N\n"
      "Note: writes to standard output!" << std::endl;
}

template<class T>
void SortAndDedupe(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

}  // namespace

int main(int argc, char *argv[]) {
  std::string arg_phase;
  std::map<std::string, Flag> flags = {
    {"phase", Flag::required(arg_phase)},
  };

  if (!ParseFlags(argc, argv, flags)) {
    std::cerr << "\n";
    PrintUsage();
    return 1;
  }

  if (argc > 1) {
    std::cerr << "Too many arguments!\n\n";
    PrintUsage();
    return 1;
  }

  InitializePerms();
  int phase = ParseInt(arg_phase.c_str());
  if (phase < 2) {
    std::cerr << "Invalid phase. Must be 2 or higher.\n";
    return 1;
  }
  if (phase % 2 != 0) {
    std::cerr << "Invalid phase. Must be an even number.\n";
    return 1;
  }

  // Open input/r(N-2).bin
  const std::string input_filename = PhaseInputFilename(phase - 2);
  RnAccessor acc(input_filename.c_str());
  if (VerifyInputChunks(phase - 2, acc) != 0) {
    std::cerr << "Failed to verify " << input_filename << std::endl;
    return 1;
  }

  // Open input/r(N-2)-new.bin
  const std::string diff_filename = PhaseDiffFilename(phase - 2);
  const size_t diff_filesize = std::filesystem::file_size(diff_filename);
  std::unique_ptr<void, std::function<void(void*)>> diff_data(
    MemMap(diff_filename.c_str(), diff_filesize, false),
    [size=diff_filesize](void *data) {
      MemUnmap(data, size);
    });
  byte_span_t diff_bytes(
    reinterpret_cast<const uint8_t*>(diff_data.get()), diff_filesize);

  std::vector<size_t> next_dedupe(num_chunks, 64000);
  std::vector<std::vector<int>> chunk_preds(num_chunks);
  REP(chunk, num_chunks) REP(part, 2) {
    std::optional<std::vector<int64_t>> ints = DecodeEF(&diff_bytes);
    if (!ints) {
      std::cerr << "Failed to decode chunk " << chunk << " part " << part << " in file: " << diff_filename << std::endl;
      return 1;
    }
    if (part == 1) {  // wins
      for (int64_t perm_index : *ints) {
        Perm perm = PermAtIndex(perm_index);
        GeneratePredecessors(perm, [&acc, &chunk_preds, &next_dedupe](const Perm &pred) {
          int64_t pred_index = IndexOf(pred);
          Outcome o = acc[pred_index];
          assert(o != LOSS);
          if (o == TIE) {
            int pred_chunk = pred_index / chunk_size;
            int pred_offset = pred_index % chunk_size;
            std::vector<int> &dst = chunk_preds[pred_chunk];
            dst.push_back(pred_offset);
            size_t &nd = next_dedupe[pred_chunk];
            if (dst.size() == nd) {
              SortAndDedupe(dst);
              std::cerr << "Deduplicating from " << nd << " to " << dst.size()
                  << " unique predecessors in output chunk " << pred_chunk << std::endl;
              nd *= 2;
            }
          }
        });
      }
      std::cerr << chunk + 1 << " / " << num_chunks << " chunks complete..." << std::endl;
    }
  }
  std::cerr << '\n';
  REP(chunk, num_chunks) {
    std::vector<int> &src = chunk_preds[chunk];
    SortAndDedupe(src);
    std::cerr << "Writing chunk " << chunk << " / " << num_chunks << " with " << src.size() << " potential losses.\n";
    std::vector<int64_t> ints;
    ints.reserve(src.size());
    for (int i : src) {
      assert(i >= 0 && i < chunk_size);
      ints.push_back(int64_t{chunk} * int64_t{chunk_size} + int64_t{i});
    }
    bytes_t bytes = EncodeEF(ints);
    if (!std::cout.write(reinterpret_cast<const char*>(bytes.data()), bytes.size())) {
      std::cerr << "write() failed!" << std::endl;
      return 1;
    }
  }
}
