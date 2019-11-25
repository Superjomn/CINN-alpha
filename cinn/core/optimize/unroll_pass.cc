#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/optimize/unroll_utils.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

struct UnrollMutator : public ir::IRMutator {
  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

  void Visit(const ir::For *op, Expr *expr) override {
    auto *node = expr->As<ir::For>();

    if (optimize::Unroller::CanUnroll(*expr, 16) || optimize::Unroller::CanUnroll(*expr, 8) ||
        optimize::Unroller::CanUnroll(*expr, 4)) {
      optimize::Unroller mutator;
      mutator(expr);
    } else {
      Visit(&node->body, &node->body);
      // no need to visit init, inc expression.
    }
  }
};

class UnrollPass : public Pass<ir::Expr> {
 public:
  explicit UnrollPass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    UnrollMutator mutator;
    mutator.Visit(expr, expr);

    ir::IRSimplify(expr);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(unroll, cinn::UnrollPass);
