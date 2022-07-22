#ifndef AUTOMATIC_SOLVER_H_INCLUDED
#define AUTOMATIC_SOLVER_H_INCLUDED

#include "client/client.h"
#include "bytes.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <vector>

class AutomaticSolver {
public:
  static constexpr int min_sleep_seconds = 5;
  static constexpr int max_sleep_seconds = 600;  // 10 minutes

  AutomaticSolver(
    std::string solver_id,
    std::string host, std::string port,
    std::string user, std::string machine,
    std::function<std::string(int, int)> chunk_file_namer,
    std::function<bytes_t(int, int)> chunk_computer,
    std::optional<int> phase);

  void Run();

private:
  std::string solver_id;
  std::string host, port, user, machine;
  std::function<std::string(int, int)> chunk_file_namer;
  std::function<bytes_t(int, int)> chunk_computer;
  std::optional<int> phase;
  bool fixed_phase = false;
  std::deque<int> chunks;
  int sleep_seconds = 0;

  ErrorOr<Client> Connect();

  void ResetSleepTime();

  void Sleep();

  bool GetCurrentPhase();

  bool GetMoreChunks();

  void ProcessChunk(int chunk);

  bool ReportChunk(int chunk, const bytes_t &bytes);

};

#endif // ndef AUTOMATIC_SOLVER_H_INCLUDED
