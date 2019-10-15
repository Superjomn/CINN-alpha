/**
 * This file implements a logging module, which is similar to tiramisu's DEBUG. It helps to print the debug information
 * with indents and make it easier to track the isl operations.
 */
#pragma once
#include <glog/logging.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define DEBUG_WITH_LOCATION

namespace cinn {
namespace utils {

#define CINN_DEBUG(level)                                                                                        \
  ::cinn::utils::Log(                                                                                            \
      __FILE__, __LINE__, ::cinn::utils::__cinn_log_indent__, ::cinn::utils::cur_log_indent_debug_level + level) \
      .stream()

extern int __cinn_log_level__;
extern int __cinn_log_indent__;

struct Log {
  Log(const char* file, int lineno, int indent, int level)
      : file_(file), lineno_(lineno), indent_(indent), level_(level) {}

  std::ostream& operator<<(const std::string& x) { return stream(); }

  std::ostream& stream() { return os_; }

  ~Log() {
    std::string log = os_.str();
    if (level_ > __cinn_log_level__ || log.empty()) return;
#ifdef DEBUG_WITH_LOCATION
    std::string sign = "[" + file_ + ":" + std::to_string(lineno_) + "]";
    std::cerr << std::setw(70) << std::right << sign;
#endif

    std::cerr << std::left << "'";
    for (int i = 0; i < __cinn_log_indent__; i++) std::cerr << "    |";
    std::cerr << ' ' << log << std::endl;
  }

 private:
  std::stringstream os_;
  std::string file_;
  int lineno_;
  int indent_;
  int level_;
};

struct LogIndentGuard {
  LogIndentGuard() { ++__cinn_log_indent__; }
  ~LogIndentGuard() { --__cinn_log_indent__; }
};

//! This value controls the indent size of a block. It is reset by LOG_INDENT macro, all the CINN_DEBUG macro in a block
//! will calcuate their indent size from this as base.
extern int cur_log_indent_debug_level;

#define LOG_INDENT(level)                            \
  ::cinn::utils::cur_log_indent_debug_level = 0;     \
  CINN_DEBUG(level) << "func " << __FUNCTION__;      \
  ::cinn::utils::cur_log_indent_debug_level = level; \
  ::cinn::utils::LogIndentGuard ______;

}  // namespace utils
}  // namespace cinn
