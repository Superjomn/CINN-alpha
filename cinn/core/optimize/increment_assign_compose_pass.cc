#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

struct Mutator : public ir::IRMutator {
  void Visit(ir::Expr *op, ir::Expr *expr) { ir::IRMutator::Visit(op, expr); }

  void Visit(const ir::Assign *op, ir::Expr *expr) override {}
};

}  // namespace cinn
