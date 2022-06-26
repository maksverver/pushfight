#ifndef LOST_POSITIONS_H_INCLUDED
#define LOST_POSITIONS_H_INCLUDED

#include <cstdint>

const int64_t *ImmediatelyLostBegin();

const int64_t *ImmediatelyLostEnd();

bool IsImmediatelyLost(int64_t index);

#endif  // ndef LOST_POSITIONS_H_INCLUDED
