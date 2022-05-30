#ifndef FLAGS_H_INCLUDED
#define FLAGS_H_INCLUDED

#include <map>
#include <string>

struct Flag {
  enum Requirement : char { OPTIONAL, REQUIRED };

  static Flag required(std::string &value) { return Flag(value, REQUIRED); }
  static Flag optional(std::string &value) { return Flag(value, OPTIONAL); }

  Flag(std::string &value, Requirement requirement)
    : value(value), requirement(requirement) {}

  std::string &value;
  Requirement requirement;
  bool provided = false;
};

bool ParseFlags(int &argc, char **&argv, std::map<std::string, Flag> &flags);

#endif // ndef FLAGS_H_INCLUDED
