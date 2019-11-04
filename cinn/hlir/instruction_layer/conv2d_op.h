#pragma once

#include <vector>

namespace cinn {
namespace hlir {
namespace instruction_layer {

struct Conv2dParam {
  std::vector<int> strides{{1, 1}};
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
