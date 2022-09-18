#ifndef RANDOM_H_INCLUDED
#define RANDOM_H_INCLUDED

#include <random>

// Returns a Mersenne Twister pseudo-random number generator that is properly
// seeded from a random device. This should produce high-quality pseudo-random
// numbers that are different for every instance returned.
std::mt19937 InitializeRng();

#endif  // ndef RANDOM_H_INCLUDED
