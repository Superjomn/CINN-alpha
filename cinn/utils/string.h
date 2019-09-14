#pragma once

#include <stdarg.h>  // For va_start, etc.
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace cinn {

//! Concat a list of strings, similar to python's xx.join([...])
static std::string Concat(const std::vector<std::string>& fields, const std::string& splitter) {
  std::stringstream ss;
  for (int i = 0; i < fields.size() - 1; i++) {
    ss << fields[i] << splitter;
  }
  if (fields.size() >= 1) ss << fields.back();
  return ss.str();
}

//! Get the string repr by operator<<.
template <typename T>
std::string GetStreamStr(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

static std::string StringFormat(const std::string fmt_str, ...) {
  /* Reserve two times as much as the length of the fmt_str */
  int final_n, n = (static_cast<int>(fmt_str.size())) * 2;
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (1) {
    formatted.reset(new char[n]);                 /* Wrap the plain char array into the unique_ptr */
    std::strcpy(&formatted[0], fmt_str.c_str());  // NOLINT
    va_start(ap, fmt_str);
    final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return std::string(formatted.get());
}
}  // namespace cinn
