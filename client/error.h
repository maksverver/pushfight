#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#include <string>
#include <variant>

struct Error {
  explicit Error(std::string message) : message(std::move(message)) {}

  std::string message;
};

template<class T>
class ErrorOr {
public:
  using ErrorT = ::Error;

  ErrorOr(ErrorT error) : value(std::move(error)) {}
  ErrorOr(T value) : value(std::move(value)) {}

  bool IsError() const { return value.index() == 0; }
  bool IsValue() const { return value.index() == 1; }

  operator bool() const { return IsValue(); }

  ErrorT &Error() { return std::get<0>(value); }
  const ErrorT &Error() const { return std::get<0>(value); }

  T &Value() { return std::get<1>(value); }
  const T &Value() const { return std::get<1>(value); }

private:
  std::variant<ErrorT, T> value;
};

#endif  // ndef ERROR_H_INCLUDED
