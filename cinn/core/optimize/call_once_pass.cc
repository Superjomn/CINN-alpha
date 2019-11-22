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

namespace {

class InsertCallOnceExprMutator : public ir::IRMutator {
 public:
  void operator()(Expr *expr) { Visit(expr, expr); }

 private:
  bool last_is_call_once_mark = false;

  void Visit(const ir::Mark *op, Expr *expr) override {
    auto *node = expr->As<ir::Mark>();
    if (node->content == _call_once_mark_) {
      node->content = "call once statement";
      last_is_call_once_mark = true;
    }
    IRMutator::Visit(op, expr);
  }

  void Visit(const ir::For *op, Expr *expr) override {
    auto *node = expr->As<ir::For>();

    if (!last_is_call_once_mark) {
      IRMutator::Visit(op, expr);
    } else {
      last_is_call_once_mark = false;
      expr->Reset(ir::CallOnce::make(ir::Block::make({*expr})));

      Visit(expr, expr);
    }
  }

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
};

//! Collect all the cond_var of the CallOnce.
struct CallOnceCondVarCollector : public ir::IRVisitor {
  std::set<std::string> operator()(const Expr *op) {
    Visit(op);
    return vars;
  }

 private:
  void Visit(const ir::CallOnce *op) override {
    vars.insert(op->cond_var_name);
    IRVisitor::Visit(op);
  }

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

  std::set<std::string> vars;
};

//! Transform all the CallOnce to IfThenElse.
struct CallOnceToIfElseMutator : public ir::IRMutator {
  void operator()(Expr *op) { Visit(op, op); }

 private:
  void Visit(const ir::CallOnce *op, Expr *expr) override {
    auto *node = expr->As<ir::CallOnce>();
    Expr cond(op->cond_var_name, primitive_t::boolean);

    auto *block = node->block.As<ir::Block>();
    auto if_node = ir::IfThenElse::make(cond, op->block);

    Expr cond_var(op->cond_var_name, primitive_t::boolean);

    block->body.push_back(ir::Assign::make(cond_var, Expr(false)));

    expr->Reset(if_node);
    Visit(expr, expr);
  }

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
};

//! Module create the conditional variable of all the CallOnce nodes.
void ModuleCreateAllCondVars(ir::Expr *expr, const std::set<std::string> &vars) {
  LOG_INDENT(0);
  CINN_DEBUG(2) << "to create " << vars.size() << " conditional variables";
  CHECK(expr->is_module());
  auto *module = expr->As<ir::Module>();

  auto &data_body = module->global_data_section.As<ir::Block>()->body;
  for (auto var : vars) {
    auto let_expr = ir::Let::make(Expr(var, primitive_t::boolean), Expr(true));
    data_body.push_back(let_expr);
  }
}

}  // namespace

class CallOnceProcessPass : public Pass<ir::Expr> {
 public:
  explicit CallOnceProcessPass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    LOG_INDENT(0);
    CHECK(expr->is_module());

    {
      InsertCallOnceExprMutator mutator;
      mutator(expr);
    }

    CallOnceCondVarCollector collector;
    auto cond_vars = collector.operator()(expr);
    CINN_DEBUG(1) << "collect " << cond_vars.size() << " conditional variables";

    CallOnceToIfElseMutator mutator;
    mutator(expr);

    ModuleCreateAllCondVars(expr, cond_vars);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(call_once_process, cinn::CallOnceProcessPass);
