#pragma once

#include <stdarg.h>  // For va_start, etc.
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace cinn {

//! Concat a list of strings, similar to python's xx.join([...])
template <typename T>
static std::string Concat(const T& fields, const std::string& splitter) {
  std::vector<std::string> ordered(fields.begin(), fields.end());
  std::stringstream ss;
  for (int i = 0; i < ordered.size() - 1; i++) {
    ss << ordered[i] << splitter;
  }
  if (ordered.size() >= 1) ss << ordered.back();
  return ss.str();
}

/**
 * Convert a container of some type to a vector of string.
 * @tparam Container container type such as std::vector.
 * @param vs the container instance.
 * @return a vector of string.
 */
template <typename Container>
std::vector<std::string> ToString(const Container& vs) {
  std::vector<std::string> res;
  for (auto& x : vs) {
    res.push_back(std::to_string(x));
  }
  return res;
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
