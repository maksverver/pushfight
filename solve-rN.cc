#include "accessors.h"
#include "macros.h"

#include <iostream>

int main() {
  // TODO: implement the rN solver here (start by copying from solve-r1).
  // For now, this just contains a small test of the R1Accessor.
  R1Accessor accessor;
  REP(i, 1000) {
    if (i > 0 && i%50 == 0) std::cout << std::endl;
    if (i%5 == 0) std::cout << ' ';
    Outcome o = accessor[i];
    std::cout << "TLW"[o];
  }
  std::cout << std::endl;

  FOR(i, 2*chunk_size - 500, 2*chunk_size) {
    if (i > 0 && i%50 == 0) std::cout << std::endl;
    if (i%5 == 0) std::cout << ' ';
    Outcome o = accessor[i];
    std::cout << "TLW"[o];
  }
  std::cout << std::endl;

  FOR(i, 2*chunk_size, 2*chunk_size + 500) {
    if (i > 0 && i%50 == 0) std::cout << std::endl;
    if (i%5 == 0) std::cout << ' ';
    Outcome o = accessor[i];
    std::cout << "TLW"[o];
  }
  std::cout << std::endl;
}
