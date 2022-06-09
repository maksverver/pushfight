#include "parse-int.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

int ParseInt(const char *s) {
  std::istringstream iss(s);
  int64_t i = -1;
  if (!(iss >> i)) {
    std::cerr << "Could not parse string (" << s << ") as an integer." << std::endl;
    exit(1);
  }
  return i;
}


int64_t ParseInt64(const char *s) {
  std::istringstream iss(s);
  int64_t i = -1;
  if (!(iss >> i)) {
    std::cerr << "Could not parse string (" << s << ") as a 64-bit integer." << std::endl;
    exit(1);
  }
  return i;
}
