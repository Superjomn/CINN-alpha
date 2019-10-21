#include "cinn/utils/logging.h"

namespace cinn {
namespace utils {

int __cinn_log_level__{3};
int __cinn_log_indent__{};
int cur_log_indent_debug_level;
int log_last_level;  // cache for jump out of a scope.

}  // namespace utils
}  // namespace cinn
