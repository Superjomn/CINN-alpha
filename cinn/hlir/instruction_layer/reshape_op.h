#pragma once

#include <vector>
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

struct ReshapeParam {
  std::vector<int> shape;
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
