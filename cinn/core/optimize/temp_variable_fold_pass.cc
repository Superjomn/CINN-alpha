/**
 * The tmp_varialbe_fold_pass defines a pass that can optimize the temporary buffers.
 */

#include <set>
#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/transform/transforms.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

/**
 * Collect all the reference expressions in the left of Assign expressions.
 */
struct CollectAssignedReference : public ir::IRVisitor {
  std::set<std::string> expr_keys;
  std::vector<ir::Expr> exprs;

  std::vector<ir::Expr> operator()(const Expr &op) {
    Visit(&op);
    return exprs;
  }

 private:
  void Visit(const Expr *op) override { IRVisitor::Visit(op); }
  void Visit(const ir::Assign *op) override {
    if (op->a.is_reference()) {
      auto key = ir::Dump(op->a);
      if (!expr_keys.count(key)) exprs.push_back(op->a);
      expr_keys.insert(key);
    }
    IRVisitor::Visit(op);
  }
};

/**
 * Naive way to tell a temporary var is foldable.
 *
 * Currently, the naive case:
 *    a = ...
 *    b = a ...
 *
 *    will be folded to "b = ..." and the variable "a" will be removed.
 *
 * that is, a variable (Reference expression) is foldable only if
 *   1. it is the left argument of an expression by once (mutated)
 *   2. it is the right argument of an expression by once (read)
 */
struct IsTempVarFoldable : public ir::IRVisitor {
  bool reach_assigned = false;
  bool reach_read = false;
  int result = -1;
  int reach_count = 0;
  // -1 for left, 1 for right.
  int left_or_right = 0;
  ir::Expr var;

  explicit IsTempVarFoldable(ir::Expr var) : var(var) {}

  bool operator()(ir::Expr expr) {
    Visit(&expr);
    return reach_count == 2 && reach_read && reach_assigned;
  }

 private:
  void Visit(const Expr *op) override {
    if (ir::Dump(*op) == ir::Dump(var)) {
      reach_count++;
      if (left_or_right == -1) {
        reach_assigned = true;
      } else if (left_or_right == 1) {
        reach_read = true;
      }
    } else {
      IRVisitor::Visit(op);
    }
  }
  void Visit(const ir::Assign *op) override {
    left_or_right = -1;
    Visit(&op->a);

    left_or_right = 1;
    Visit(&op->b);
  }
};

/**
 * Fold a temporary variable.
 */
struct FoldTempVariable : public ir::IRMutator {
  void operator()(Expr *expr) { Visit(expr, expr); }
  std::map<std::string, ir::Expr> local_foldable_vars;

 private:
  void Visit(const ir::For *op, Expr *expr) override {
    auto *node = expr->As<ir::For>();
    auto *block = node->body.As<ir::Block>();

    local_foldable_vars.clear();
    CollectLocallyFoldableVars(*expr, block);
    FoldTempVarInBlock(&node->body);

    IRMutator::Visit(op, expr);
  }

  void FoldTempVarInBlock(ir::Expr *block_expr) {
    LOG_INDENT(0);
    ir::Block *block = block_expr->As<ir::Block>();

    std::vector<ir::Expr> new_body;
    std::map<std::string, ir::Expr> tmp_vars;

    for (auto expr : block->body) {
      if (expr.is_assign() && expr.As<ir::Assign>()->a.is_reference()) {
        tmp_vars[ir::Dump(expr)] = expr;
      }
      new_body.push_back(expr);
    }

    auto new_block = ir::Block::make(std::move(new_body));
    for (auto &item : tmp_vars) {
      auto *assign = item.second.As<ir::Assign>();
      if (local_foldable_vars.count(ir::Dump(assign->a))) {
        FoldTempVarInBlock(assign->a, &new_block);
      }
    }
    block_expr->Reset(new_block);
  }

  void FoldTempVarInBlock(ir::Expr var, ir::Expr *block_expr) {
    auto *block = block_expr->As<ir::Block>();
    std::vector<ir::Expr> new_body;

    ir::Expr to;

    for (auto expr : block->body) {
      auto *assign = expr.As<ir::Assign>();
      if (assign && ir::IREquals(assign->a, var)) {
        to.Reset(assign->b);
      } else {
        ir::IRReplace(&expr, var, to);
        new_body.push_back(expr);
      }
    }

    block_expr->Reset(ir::Block::make(std::move(new_body)));
  }

  void CollectLocallyFoldableVars(ir::Expr expr, ir::Block *block) {
    LOG_INDENT(0);
    local_foldable_vars.clear();
    std::vector<ir::Expr> refs = CollectAssignedReference()(expr);
    for (auto &ref : refs) {
      if (IsVarLocallyFoldable(ref, expr)) {
        CINN_DEBUG(2) << "collect locally foldable var " << ref;
        local_foldable_vars[ir::Dump(ref)] = ref;
      }
    }
  }

  bool IsVarLocallyFoldable(const ir::Expr &var, ir::Expr block) {
    IsTempVarFoldable teller(var);
    return teller(block);
  }

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
};

class TempVariableFoldPass : public Pass<ir::Expr> {
 public:
  explicit TempVariableFoldPass(const std::string &name) : Pass(name) {}

  void Impl(Expr *expr) override { FoldTempVariable()(expr); }
};

}  // namespace cinn

REGISTER_IR_PASS(temp_variable_fold, cinn::TempVariableFoldPass);
