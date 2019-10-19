#include "cinn/core/optimize/optimizer.h"
#include <cinn/ir/ir.h>
#include "cinn/core/optimize/pass_registry.h"

namespace cinn {

void Optimizer::operator()(ir::Expr* expr) {
  for (auto& name : passes_in_order_) {
    auto* pass = PassRegistry::Global().GetPass(name);
    CHECK(pass) << "pass [" << name << "] not exists";
    pass->Run(expr);
  }
}

Optimizer::Optimizer() {
  passes_in_order_.assign({
      "nested_block_clean", "indices_to_absolute_offset", "fold_reference_indices",
  });
}

}  // namespace cinn
