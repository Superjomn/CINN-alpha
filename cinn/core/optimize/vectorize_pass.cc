#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

namespace {

/**
 * Collect arguments of SIMD instructions.
 */
class SIMDArgCollector : public ir::IRVisitor {
  //! Arguments of SIMD operations that need to be cast.
  std::map<std::string, Expr> args_;

  std::stack<bool> inside_simd_;

 public:
  SIMDArgCollector() = default;

  const std::map<std::string, Expr> &args() { return args_; }

  void Visit(const ir::Assign *op) override {
    // A[i] = simd_op(xx)
    // then `A[i]` should be casted:
    //  simd_d& a = cast(A[i]);
    //  a = simd_op(xx)
    if (op->b.is_simd_opr()) {
      args_[ir::Dump(op->a)] = op->a;
    }
    ir::IRVisitor::Visit(op);
  }

  void Visit(const ir::SIMDOpr *op) override {
    if (op->a.is_reference()) {
      args_[ir::Dump(op->a)] = op->a;
    }
    if (op->b.is_reference()) {
      args_[ir::Dump(op->b)] = op->b;
    }

    ir::IRVisitor::Visit(op);
  }

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }
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
      // Remove the outer forloop and reset the iterator to zero.
      auto *for_ = expr->As<ir::For>();
      auto iter = for_->iterator;
      auto &for_block = for_->body;
      *expr = for_block;

      // TODO(Superjomn) replace all with the global zero constant.
      ir::IRVarReplacer replacer(iter, ir::Expr(0));
      replacer.Visit(expr, expr);

      Visit(expr->As<ir::Block>(), expr);

      {
        // Collect the arguments of the SIMD intrics and add Cast operator to transform the original References to SIMD
        // data types such as __m128.
        SIMDArgCollector collector;
        collector.Visit(expr);
        LOG(INFO) << "Collect references from \n" << ir::Dump(*expr);
        LOG(INFO) << "collected reference nums: " << collector.args().size();

        BlockPreappendSimdCastedArguments(expr->As<ir::Block>(), collector.args());
      }
    } else {
      IRMutator::Visit(op, expr);
    }
  }

  void BlockPreappendSimdCastedArguments(ir::Block *block, const std::map<std::string, ir::Expr> &args) {
    LOG_INDENT(0);
    for (auto &item : args) {
      auto let_expr = CreateSimdCastExpr(ir::IRDeepCopy(item.second));
      CINN_DEBUG(2) << "Insert Let record: " << ir::Dump(let_expr);
      LOG(INFO) << "block.size " << block->exprs.size();
      block->exprs.push_back(let_expr);
      // block->exprs.insert(std::begin(block->exprs), std::move(let_expr));
      // CINN_DEBUG(2) << "Insert let expr: " << ir::Dump(let_exprs.back());
    }
  }

  ir::Expr CreateSimdCastExpr(ir::Expr expr) {
    CHECK_EQ(vector_width, 4);
    LOG(INFO) << "create simd cast expr";
    auto cast_op = ir::Cast::make(expr, expr.ptype(), composite_t::simd128);
    ir::Var new_var(NameGenerator::Global().NewVarName());
    new_var.set_ptype(expr.ptype());
    new_var.set_ctype(composite_t::simd128);
    new_var.set_is_reference();
    // auto let_expr = ir::Let::make(Expr(new_var), cast_op);
    // let_expr.set_ctype(new_var.ctype());
    // LOG(INFO) << "let_expr: " << ir::Dump(let_expr);
    // CHECK(let_expr.As<ir::Let>()->a.As<ir::Var>()->is_reference());
    // return let_expr;
    return new_var;
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
        // CHECK(for_expr.is_for_());
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
};

class VectorizePass : public Pass<ir::Expr> {
 public:
  explicit VectorizePass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    VectorizeMutator mutator;
    mutator.Visit(expr, expr);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(vectorize, cinn::VectorizePass);
