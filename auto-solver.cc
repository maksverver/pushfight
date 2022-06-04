#include "auto-solver.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "bytes.h"

AutomaticSolver::AutomaticSolver(
  std::string solver_id, int phase,
  std::string host, std::string port,
  std::string user, std::string machine,
  std::function<std::string(int)> chunk_file_namer,
  std::function<bytes_t(int)> chunk_computer)
  : solver_id(std::move(solver_id)), phase(phase),
    host(std::move(host)), port(std::move(port)),
    user(std::move(user)), machine(std::move(machine)),
    chunk_file_namer(std::move(chunk_file_namer)),
    chunk_computer(std::move(chunk_computer)) {
  std::cout << "Automatic solver " << solver_id << " using host " << host << " port " << port << std::endl;
}

void AutomaticSolver::Run() {
  for (;;) {
    if (chunks.empty()) {
      if (GetMoreChunks()) {
        ResetSleepTime();
      } else {
        Sleep();
      }
    } else {
      int chunk = chunks.front();
      chunks.pop_front();
      ProcessChunk(chunk);
    }
  }
}

ErrorOr<Client> AutomaticSolver::Connect() {
  return Client::Connect(phase, host.c_str(), port.c_str(), solver_id, user, machine);
}

void AutomaticSolver::ResetSleepTime() {
  sleep_seconds = 0;
}

void AutomaticSolver::Sleep() {
  if (sleep_seconds == 0) {
    sleep_seconds = min_sleep_seconds;
  } else {
    sleep_seconds = std::min(sleep_seconds * 2, max_sleep_seconds);
  }
  std::cout << "Sleeping for " << sleep_seconds << " seconds before retrying..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
}

bool AutomaticSolver::GetMoreChunks() {
  std::cout << "Queue is empty. Fetching more chunks from the server at "
      << host << ":" << port << "..." << std::endl;
  if (auto client = Connect(); !client) {
    std::cerr << "Failed to connect: " << client.Error().message << std::endl;
  } else if (auto chunks = client->GetChunks(); !chunks) {
    std::cerr << "Failed to get chunks: " << chunks.Error().message << std::endl;
  } else if (chunks->empty()) {
    std::cerr << "Server has no more chunks available!" << std::endl;
  } else {
    std::cout << "Server returned " << chunks->size() << " more chunks to solve." << std::endl;
    this->chunks.insert(this->chunks.end(), chunks->begin(), chunks->end());
    return true;
  }
  return false;
}

void AutomaticSolver::ProcessChunk(int chunk) {
  const std::string filename = chunk_file_namer(chunk);
  bytes_t bytes;
  if (std::filesystem::exists(filename) && std::filesystem::file_size(filename) > 0) {
    std::cout << "Chunk output already exists. Loading..." << std::endl;
    bytes = ReadFromFile(filename);
    assert(!bytes.empty());
  } else {
    std::cout << "Calculating chunk " << chunk << std::endl;
    bytes = chunk_computer(chunk);
    WriteToFile(filename, bytes);
  }
  std::cout << "Chunk complete! Reporting result to server..." << std::endl;
  ReportChunk(chunk, bytes);
}

bool AutomaticSolver::ReportChunk(int chunk, const bytes_t &bytes) {
  if (auto client = Connect(); !client) {
    std::cerr << "Failed to connect: " << client.Error().message << std::endl;
    return false;
  } else if (ErrorOr<size_t> uploaded = client->SendChunk(chunk, bytes); !uploaded) {
    std::cout << "Failed to send result to server: " << uploaded.Error().message << std::endl;
    return false;
  } else if (*uploaded == 0) {
    std::cout << "Succesfully reported result to server! (No upload required.)" << std::endl;
    return true;
  } else {
    std::cout << "Succesfully uploaded chunk to server! "
        << "(" << bytes.size() << " bytes; " << *uploaded << " bytes compressed)" << std::endl;
    return true;
  }
}
