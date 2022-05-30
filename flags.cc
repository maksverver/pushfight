#include "flags.h"

#include <cstring>
#include <iostream>

// Parses all flags of the form --key=value and removes them from the argument list.
bool ParseFlags(int &argc, char **&argv, std::map<std::string, Flag> &flags) {
  int pos_out = 1;
  for (int pos_in = 1; pos_in < argc; ++pos_in) {
    char *arg = argv[pos_in];
    if (arg[0] == '-') {
      if (arg[1] != '-') {
        std::cerr << "Invalid argument: " << arg << std::endl;
        return false;
      }
      const char *start = arg + 2;
      const char *sep = strchr(start, '=');
      std::string key;
      std::string value;
      if (!sep) {
        key = std::string(start);
        value = "true";
      } else {
        key = std::string(start, sep);
        value = std::string(sep + 1);
      }
      if (auto it = flags.find(key); it == flags.end()) {
        std::cerr << "Unknown flag: " << key << std::endl;
        return false;
      } else if (it->second.provided) {
        std::cerr << "Duplicate value for flag: " << key << std::endl;
        return false;
      } else {
        it->second.provided = true;
        it->second.value = std::move(value);
      }
    } else {
      argv[pos_out++] = arg;
    }
  }

  // Delete removed arguments.
  for (int i = pos_out; i < argc; ++i) argv[i] = nullptr;
  argc = pos_out;

  // Check for missing flags.
  bool missing = false;
  for (auto const &entry : flags) {
    const Flag &flag = entry.second;
    if (flag.requirement == Flag::REQUIRED && !flag.provided) {
      std::cerr << "Missing required flag --" << entry.first << "!" << std::endl;
      missing = true;
    }
  }
  return !missing;
}
