#pragma once

#include <vector>
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {
namespace instruction_layer {

struct PadParam {
  std::vector<std::vector<ir::Constant>> padding;

  enum class PaddingType {
    kConstant = 0,
    kReflect,
    kSymmetric,
  };
  PaddingType type;
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
