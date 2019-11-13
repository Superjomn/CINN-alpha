#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

namespace {

struct Mutator : public ir::IRMutator {
  void Visit(const Expr* op, Expr* expr) override { IRMutator::Visit(op, expr); }

  void Visit(const ir::Block* op, Expr* expr) override {
    auto* a = expr->As<ir::Block>();

    bool detect_nested = false;
    std::vector<Expr> new_exprs;
    for (auto& e : a->body) {
      if (e.is_block()) {
        detect_nested = true;
        for (auto& e1 : e.As<ir::Block>()->body) {
          new_exprs.push_back(e1);
        }
      } else {
        new_exprs.push_back(e);
      }
    }

    if (detect_nested) {
      a->body = new_exprs;

      Visit(op, expr);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }
};

}  // namespace

class NestedBlockCleanPass : public Pass<ir::Expr> {
 public:
  explicit NestedBlockCleanPass(const std::string& name) : Pass(name) {}

  void Impl(ir::Expr* expr) override {
    Mutator mutator;
    mutator.Visit(expr, expr);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(nested_block_clean, cinn::NestedBlockCleanPass);
