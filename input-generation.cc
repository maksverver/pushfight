#include "accessors.h"
#include "byte_span.h"
#include "efcodec.h"
#include "input-verification.h"
#include "macros.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <functional>
#include <optional>

// Generates an input file from a previous one and a list of new losses and wins.
//
// Let's say we have:
//
//   - input_filename = input/r12.bin
//   - temp_filename = input/r12.bin.tmp
//   - previous_input_filename = input/r10.bin
//   - diff_filename = input/r12-new.bin
//
// The procedure is as follows:
//
//   - rename input/r10.bin to input/r10.bin.tmp
//   - apply diff to input/r10.bin.tmp
//   - verify the generated input is correct
//   - rename input/r10.bin.tmp to input/r10.bin
//
bool GeneratePhaseInput(
    int phase,
    const char *input_filename,
    const char *temp_filename,
    const char *previous_input_filename,
    const char *diff_filename) {
  if (std::filesystem::exists(input_filename)) {
    std::cerr << "Using existing input file " << input_filename << std::endl;
    return true;
  }

  if (!std::filesystem::exists(temp_filename) &&
      !std::filesystem::exists(previous_input_filename)) {
    std::cerr << "Cannot generate " << input_filename << "; missing previous input file " << previous_input_filename << std::endl;
    return false;
  }

  if (!std::filesystem::exists(diff_filename)) {
    std::cerr << "Cannot generate " << input_filename << "; missing diff file " << diff_filename << std::endl;
    return false;
  }

  size_t diff_filesize = std::filesystem::file_size(diff_filename);
  if (diff_filesize == 0) {
    std::cerr << "Diff is empty!" << std::endl;
    return false;
  }

  if (!std::filesystem::exists(temp_filename)) {
    std::filesystem::rename(previous_input_filename, temp_filename);
  }

  std::unique_ptr<void, std::function<void(void*)>> diff_data(
      MemMap(diff_filename, diff_filesize, false),
      [size=diff_filesize](void *data) {
        MemUnmap(data, size);
      });
  byte_span_t diff_bytes(
    reinterpret_cast<const uint8_t*>(diff_data.get()), diff_filesize);

  // Step 1: integrate new wins and losses.
  std::cerr << "Generating " << input_filename << " from "
      << previous_input_filename << " and " << diff_filename << "..." << std::endl;
  {
    MutableRnAccessor acc(temp_filename);
    int64_t losses = 0, wins = 0, new_losses = 0, new_wins = 0;
    REP(chunk, num_chunks) REP(part, 2) {
      int64_t changes = 0;
      const char *what = part == 0 ? "losses" : "wins";
      Outcome new_outcome = part == 0 ? LOSS : WIN;
      std::optional<std::vector<int64_t>> ints = DecodeEF(&diff_bytes);
      if (!ints) {
        std::cerr << "Failed to decode chunk " << chunk << " " << what << " in file: " << diff_filename << std::endl;
        return 1;
      }
      for (int64_t i : *ints) {
        Outcome o = acc[i];
        if (o == new_outcome) {
          continue;
        }
        if (o != TIE) {
          std::cerr << temp_filename << ": Permutation " << i << " is marked " << OutcomeToString(o)
              << " should be " << OutcomeToString(new_outcome) << std::endl;
          return 1;
        }
        acc[i] = new_outcome;
        ++changes;
      }
      (part == 0 ? losses : wins) += ints->size();
      (part == 0 ? new_losses : new_wins) += changes;
      std::cerr << "Chunk " << chunk << " / " << num_chunks << ": "
          << losses << " losses (" << new_losses << " new), "
          << wins << " wins (" << new_wins << " new).\r";
    }
  }

  // Step 2: verify all chunks are correct.
  {
    std::cerr << "\nVerifying generated input..." << std::endl;
    // A little silly, but we have to reopen the file with RnAccessor
    // because VerifyInputChunks() doesn't accept MutableRnAccessor.
    RnAccessor acc(temp_filename);
    int failures = VerifyInputChunks(phase - 2, acc, num_chunks);
    if (failures != 0) {
      std::cerr << failures << " verification failures!" << std::endl;
      return false;
    }
  }

  // Step 3: rename temp file.
  std::filesystem::rename(temp_filename, input_filename);
  std::cerr << "Succesfully generated " << input_filename << "!" << std::endl;
  return true;
}
