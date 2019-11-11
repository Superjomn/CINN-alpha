#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

namespace {

/**
 * Collect the arguments of SIMDOpr in a single block(not nested).
 */
struct SimdReferenceCollector : public ir::IRVisitor {
  std::map<std::string, Expr> arguments;

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

  void Visit(const ir::Assign *op) override {
    if (op->b.is_simd_opr()) {
      CHECK(op->a.is_reference());
      Collect(op->a);
    }

    ir::IRVisitor::Visit(op);
  }

  void Visit(const ir::SIMDOpr *op) override {
    if (op->a.is_reference()) {
      Collect(op->a);
    }
    if (op->b.is_reference()) {
      Collect(op->b);
    }

    ir::IRVisitor::Visit(op);
  }

  void Visit(const ir::Block *op) override {
    for (auto &expr : op->exprs) {
      if (!expr.is_block()) {
        // This should works with the "nested_block_clean_pass".
        Visit(&expr);  // avoid nested
      }
    }
  }

  void Collect(const ir::Expr &a) { arguments[ir::Dump(a)] = a; }
};

struct SimdArgumentCastInsertToBlock : public ir::IRMutator {
  std::map<std::string, ir::Expr> arguments;

  SimdArgumentCastInsertToBlock() {}

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
  void Visit(const ir::Block *op, Expr *expr) override {
    auto *block = expr->As<ir::Block>();
    SimdReferenceCollector collector;
    collector.Visit(expr);
    if (!collector.arguments.empty()) {
      PreappendCastToBlock(expr->As<ir::Block>(), collector.arguments);
    }
    IRMutator::Visit(op, expr);
  }

 protected:
  void PreappendCastToBlock(ir::Block *block, const std::map<std::string, ir::Expr> &simd_args) {
    for (auto &item : simd_args) {
      auto cast = ir::Cast::make(item.second, item.second.ptype(), composite_t::simd128);
      cast.set_ctype(composite_t::simd128);
      ir::Var var(NameGenerator::Global().NewVarName());
      var.set_is_reference();
      auto let = ir::Let::make(Expr(var), cast);
      let.set_ctype(composite_t::simd128);
      block->exprs.insert(std::begin(block->exprs), let);
    }
  }
};

}  // namespace

class VectorizeMutator : public ir::IRMutator {
  int vector_width{-1};

 public:
  VectorizeMutator() = default;

  void Visit(const ir::Add *op, ir::Expr *expr) override {
    if (to_vectorize_ && !inside_reference_) {
      Vectorize(expr);
      ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }
  void Visit(const ir::Sub *op, ir::Expr *expr) override {
    if (to_vectorize_ && !inside_reference_) {
      Vectorize(expr);
      ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }
  void Visit(const ir::Mul *op, ir::Expr *expr) override {
    if (to_vectorize_ && !inside_reference_) {
      Vectorize(expr);
      ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }
  void Visit(const ir::Div *op, ir::Expr *expr) override {
    if (to_vectorize_ && !inside_reference_) {
      Vectorize(expr);
      ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }
  void Visit(const ir::Reference *op, ir::Expr *expr) override {
    inside_reference_ = true;
    ir::IRMutator::Visit(op, expr);
    inside_reference_ = false;
  }
  void Visit(const ir::For *op, Expr *expr) override {
    if (to_vectorize_) {
      auto *for_ = expr->As<ir::For>();
      auto iter = for_->iterator;
      auto &for_block = for_->body;
      *expr = for_block;

      // TODO(Superjomn) replace all with the global zero constant.
      ir::IRVarReplacer replacer(iter, ir::Expr(0));
      replacer.Visit(expr, expr);

      Visit(expr->As<ir::Block>(), expr);
    } else {
      IRMutator::Visit(op, expr);
    }
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *block = expr->As<ir::Block>();
    for (int i = 0; i < block->exprs.size(); i++) {
      auto &expr = block->exprs[i];
      if (expr.is_mark() && Contains(expr.As<ir::Mark>()->content, "vectorize - points")) {
        if (i + 1 >= block->exprs.size() || !block->exprs[i + 1].is_for_()) continue;
        auto &for_expr = block->exprs[i + 1];
        if (!ForGetExtent(for_expr.As<ir::For>())) continue;
        i++;

        to_vectorize_ = true;
        Visit(&for_expr, &for_expr);
        to_vectorize_ = false;
      } else {
        Visit(&expr, &expr);
      }
    }
  }

  /*
  void Visit(const ir::For *op, Expr *expr) override {
    if (to_vectorize_) {
      LOG(INFO) << "vectorize for:\n" << ir::Dump(*expr);
      auto *node = expr->As<ir::For>();
      ForGetExtent(node);
    }
    ir::IRMutator::Visit(op, expr);
  }
  */

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

  bool ForGetExtent(ir::For *op) {
    LOG_INDENT(0);
    ir::Expr cond = op->iter_cond;
    if (!cond.is_le()) return false;
    auto *le = cond.As<ir::LE>();
    if (!le->a.is_var() || !le->b.is_int_imm()) return false;
    this->vector_width = le->b.As<ir::IntImm>()->val() + 1;
    return true;
  }

  void Vectorize(Expr *expr) {
    LOG_INDENT(0);
    CINN_DEBUG(2) << "*********** Vectorize " << ir::Dump(*expr);
    CHECK_GT(vector_width, 1);
    switch (expr->type()) {
#define __(op__)                                                                      \
  case ir::NodeTy::op__: {                                                            \
    auto *op = expr->As<ir::op__>();                                                  \
    *expr = ir::SIMDOpr::make(vector_width, ir::SIMDOpr::Opr::k##op__, op->a, op->b); \
  } break;
      __(Add)
      __(Sub)
      __(Mul)
      __(Div)
#undef __
      default:
        LOG(ERROR) << "unsupported " << expr->type();
    }

    LOG(INFO) << "after vectorize " << ir::Dump(*expr);
  }

 private:
  bool to_vectorize_{false};
  bool inside_reference_{false};
  bool is_block_subsequent_vectorize_for_{false};
};

class VectorizePass : public Pass<ir::Expr> {
 public:
  explicit VectorizePass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    {
      VectorizeMutator mutator;
      mutator.Visit(expr, expr);
    }

    {
      SimdArgumentCastInsertToBlock mutator;
      mutator.Visit(expr, expr);
    }
  }
};

}  // namespace cinn

REGISTER_IR_PASS(vectorize, cinn::VectorizePass);
