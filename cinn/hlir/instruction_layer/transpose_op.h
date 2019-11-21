#pragma once
#include <vector>

namespace cinn {
namespace hlir {
namespace instruction_layer {

struct TransposeParam {
  // the permuted dimensions.
  std::vector<int> perm;
};

}  // namespace instruction_layer
}  // namespace hlir
}  // namespace cinn
