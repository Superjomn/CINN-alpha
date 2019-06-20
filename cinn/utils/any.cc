#include "cinn/utils/any.h"

namespace cinn {

size_t Any::kInvalidType{typeid(void).hash_code()};

}  // namespace cinn
