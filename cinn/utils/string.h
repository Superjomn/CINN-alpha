#pragma once

#include <stdarg.h>  // For va_start, etc.
#include <cstring>
#include <functional>
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
template <typename T>
static std::string Join(const std::vector<T>& fields,
                        const std::string& splitter,
                        std::function<std::string(const T&)>&& trans) {
  std::stringstream ss;
  for (int i = 0; i < fields.size() - 1; i++) {
    ss << trans(fields[i]) << splitter;
  }
  if (fields.size() >= 1) ss << trans(fields.back());
  return ss.str();
}

//! Replace a substr 'from' to 'to' in string s.
static void Replace(std::string* s, const std::string& from, const std::string& to) {
  size_t pos = 0;
  while ((pos = s->find(from, pos)) != std::string::npos) {
    s->replace(pos, from.size(), to);
    pos += to.length();
  }
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

/**
 * Returns whether the string `s` contains the `str`.
 * @param s
 * @param str
 * @return
 */
static bool Contains(const std::string& s, const std::string& str) {
  auto it = s.find(str);
  return it != std::string::npos;
}

static std::string Trim(const std::string& s) {
  const char* space = "\n\t\r";
  size_t begin = s.find_first_not_of(space);
  size_t end = s.find_last_not_of(space);
  return s.substr(begin, end - begin + 1);
}

}  // namespace cinn
