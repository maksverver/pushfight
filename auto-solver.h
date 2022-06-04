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
    std::string solver_id, int phase,
    std::string host, std::string port,
    std::string user, std::string machine,
    std::function<std::string(int)> chunk_file_namer,
    std::function<bytes_t(int)> chunk_computer);

  void Run();

private:

  std::string solver_id;
  int phase = -1;
  std::string host, port, user, machine;
  std::deque<int> chunks;
  int sleep_seconds = 0;
  std::function<std::string(int)> chunk_file_namer;
  std::function<bytes_t(int)> chunk_computer;

  ErrorOr<Client> Connect();

  void ResetSleepTime();

  void Sleep();

  bool GetMoreChunks();

  void ProcessChunk(int chunk);

  bool ReportChunk(int chunk, const bytes_t &bytes);

};

#endif // ndef AUTOMATIC_SOLVER_H_INCLUDED
