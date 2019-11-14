#include "cinn/utils/name_generator.h"
#include <cstddef>

namespace cinn {

void NameGenerator::ResetCounter() {
  func_counter_ = 0;
  stage_counter_ = 0;
  iterator_counter_ = 0;
  parameter_counter_ = 0;
  var_counter_ = 0;
  named_counter_ = 0;
  buffer_counter_ = 0;
  array_counter_ = 0;
  tmp_var_counter_ = 0;
}

NameGenerator &NameGenerator::Global() {
  static NameGenerator x;
  return x;
}

}  // namespace cinn