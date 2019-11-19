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
 * Collect the arguments of SIMDOpr in a single block(not nested).
 */
struct SimdReferenceCollector : public ir::IRVisitor {
  std::map<std::string, Expr> arguments;

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

  void Visit(const ir::Assign *op) override {
    if (op->b.is_simd_opr()) {
      if (op->a.is_reference()) {
        Collect(op->a);
      }
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
    for (auto &expr : op->body) {
      if (!expr.is_block()) {
        // This should works with the "nested_block_clean_pass".
        Visit(&expr);  // avoid nested
      }
    }
  }

  void Collect(const ir::Expr &a) {
    auto key = ir::Dump(a);
    if (arguments.count(key)) {
      arguments[key].set_ptr(a.ptr());
    } else {
      arguments[ir::Dump(a)] = a;
    }
  }
};

struct SimdArgumentReplacer : public ir::IRMutator {
  const std::map<std::string, ir::Expr /* var */> &ref_repr_to_var;

  explicit SimdArgumentReplacer(const std::map<std::string, ir::Expr> &x) : ref_repr_to_var(x) {}

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
  void Visit(const ir::Assign *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Assign>();
    if (op->b.is_simd_opr()) {
      ReplaceSimdArgument(&node->a);
    }

    IRMutator::Visit(&node->b, &node->b);
  }

  void Visit(const ir::SIMDOpr *op, ir::Expr *expr) override {
    LOG(INFO) << "******** Visit simd";
    auto *node = expr->As<ir::SIMDOpr>();
    LOG(INFO) << "a: " << node->a;
    LOG(INFO) << "b: " << node->b;
    ReplaceSimdArgument(&node->a);
    ReplaceSimdArgument(&node->b);

    IRMutator::Visit(op, expr);
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *node = expr->As<ir::Block>();
    for (auto &e : node->body) {
      if (!e.is_block()) {
        ir::IRMutator::Visit(&e, &e);
      }
    }
  }

 protected:
  void ReplaceSimdArgument(ir::Expr *expr) {
    for (auto &item : ref_repr_to_var) {
      if (ir::Dump(*expr) == item.first) {
        expr->set_ptr(item.second.ptr());
        break;
      }
    }
  }
};

struct SimdArgumentCastInsertToBlock : public ir::IRMutator {
  std::map<std::string, ir::Expr> arguments;
  std::vector<std::map<std::string, ir::Expr /* var */>> ref_repr_to_var;

  SimdArgumentCastInsertToBlock() {}

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
  void Visit(const ir::Block *op, Expr *expr) override {
    ref_repr_to_var.emplace_back();

    auto *block = expr->As<ir::Block>();
    SimdReferenceCollector collector;
    collector.Visit(expr);
    if (!collector.arguments.empty()) {
      PreappendCastToBlock(expr->As<ir::Block>(), collector.arguments);

      SimdArgumentReplacer replacer(ref_repr_to_var.back());
      replacer.Visit(expr, expr);
    }
    IRMutator::Visit(op, expr);

    ref_repr_to_var.pop_back();
  }

 protected:
  void PreappendCastToBlock(ir::Block *block, const std::map<std::string, ir::Expr> &simd_args) {
    for (const auto &item : simd_args) {
      LOG(INFO) << "collected " << item.first << " -> " << item.second;
      auto cast = ir::Cast::make(item.second, item.second.ptype(), composite_t::simd128);
      cast.set_ctype(composite_t::simd128);
      ir::Var var(GlobalContext().name_generator().NewVarName());
      var.set_is_reference();
      Expr var_expr(var);
      ref_repr_to_var.back()[item.first] = var_expr;

      auto let = ir::Let::make(var_expr, cast);
      let.set_ctype(composite_t::simd128);

      // Preappend cast expressions to block.
      block->body.insert(std::begin(block->body), let);
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
      ir::IRReplace(expr, iter, Expr(0));

      Visit(expr->As<ir::Block>(), expr);
    } else {
      IRMutator::Visit(op, expr);
    }
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *block = expr->As<ir::Block>();
    for (int i = 0; i < block->body.size(); i++) {
      auto &expr = block->body[i];
      if (expr.is_mark() && Contains(expr.As<ir::Mark>()->content, "vectorize - points")) {
        if (i + 1 >= block->body.size() || !block->body[i + 1].is_for_()) continue;
        auto &for_expr = block->body[i + 1];
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
      LOG(INFO) << "vectorize for:\n" << *expr;
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
    CINN_DEBUG(2) << "*********** Vectorize " << *expr;
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

    LOG(INFO) << "after vectorize " << *expr;
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

    ir::IRSimplify(expr);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(vectorize, cinn::VectorizePass);
