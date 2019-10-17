#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

/**
 * Detect the pattern of "A[e1] = A[e1] + ... and compose it into some thing like A[e1] += ..., works with other
 * operators such as - * /.
 */
struct IndicesMutator : public ir::IRMutator {
  void Visit(ir::Expr *op, ir::Expr *expr) { ir::IRMutator::Visit(op, expr); }

  void Visit(const ir::Assign *op, ir::Expr *expr) override {}
};

}  // namespace cinn
