#ifndef INPUT_GENERATION_H_INCLUDED
#define INPUT_GENERATION_H_INCLUDED

#include <functional>
#include <string>
#include <optional>

#include "client/client.h"

using client_factory_t = std::function<std::optional<Client>()>;

std::string PreparePhaseInput(int phase, const client_factory_t &client_factory);

#endif  // ndef INPUT_GENERATION_H_INCLUDED
