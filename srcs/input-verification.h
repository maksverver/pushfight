#ifndef INPUT_VERIFICATION_H_INCLUDED
#define INPUT_VERIFICATION_H_INCLUDED

#include <string>

#include "accessors.h"

// Partially verifies an input file by checking the SHA256 checksums of some
// (but not all!) chunks. Returns the number of failures.
int VerifyInputChunks(int phase, const RnAccessor &acc, int chunks_to_verify = 10);

std::string GetChecksumFilename(const char *subdir, int phase);

#endif  // ndef INPUT_VERIFICATION_H_INCLUDED
