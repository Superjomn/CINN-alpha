#include "cinn/utils/name_generator.h"
#include <cstddef>

namespace cinn {

size_t NameGenerator::func_counter_{0};
size_t NameGenerator::stage_counter_{0};
size_t NameGenerator::iterator_counter_{0};
size_t NameGenerator::parameter_counter_{0};
size_t NameGenerator::var_counter_{0};
size_t NameGenerator::named_counter_{0};
size_t NameGenerator::buffer_counter_{0};

}  // namespace cinn