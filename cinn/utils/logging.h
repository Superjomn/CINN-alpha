/**
 * This file implements a logging module, which is similar to tiramisu's DEBUG. It helps to print the debug information
 * with indents and make it easier to track the isl operations.
 */
#pragma once
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define DEBUG_WITH_LOCATION

namespace cinn {
namespace utils {

#define CINN_DEBUG(level) ::cinn::utils::Log(__FILE__, __LINE__, ::cinn::utils::__cinn_log_indent__)

extern int __cinn_log_level__;
extern int __cinn_log_indent__;

struct Log {
  Log(const char* file, int lineno, int indent) : file_(file), lineno_(lineno), indent_(indent) {}
  std::ostream& operator<<(const std::string& x) {
    stream() << x;
    return stream();
  }
  std::ostream& stream() { return os_; }

  ~Log() {
#ifdef DEBUG_WITH_LOCATION
    std::string sign = "[" + file_ + ":" + std::to_string(lineno_) + "]";
    std::cerr << std::setw(50) << std::right << sign;
#endif

    std::cerr << std::left << "'";
    for (int i = 0; i < __cinn_log_indent__; i++) std::cerr << "    |";
    std::cerr << ' ' << os_.str() << std::endl;
  }

 private:
  std::stringstream os_;
  std::string file_;
  int lineno_;
  int indent_;
};

struct LogIndentGuard {
  LogIndentGuard() { ++__cinn_log_indent__; }
  ~LogIndentGuard() { --__cinn_log_indent__; }
};

#define LOG_INDENT(info) \
  CINN_DEBUG(2) << info; \
  ::cinn::utils::LogIndentGuard ______;

}  // namespace utils
}  // namespace cinn
