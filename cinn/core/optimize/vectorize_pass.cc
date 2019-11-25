#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/core/optimize/vectorize_utils.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_mutator_helpers.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

struct VectorizeMutator : public ir::IRMutator {
  int vector_width{-1};

  VectorizeMutator() = default;

  void Visit(const ir::For *op, Expr *expr) override {
    auto *for_ = expr->As<ir::For>();
    auto iter = for_->iterator;
    auto &for_block = for_->body;

    if (to_vectorize_) {
      optimize::Vectorize vectorize;
      vectorize(vector_width, expr);
      CHECK(expr->is_block());
    } else {
      IRMutator::Visit(op, expr);
    }
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *block = expr->As<ir::Block>();
    for (int i = 0; i < block->body.size(); i++) {
      auto &cexpr = block->body[i];

      // reatch a vectorize mark, tell that the following forloop(or the forloop inside it) can be vectorized.
      if (cexpr.is_mark() && Contains(cexpr.As<ir::Mark>()->content, "vectorize - points")) {
        reatch_vectorize_mark_ = true;
      } else if (reatch_vectorize_mark_ && cexpr.is_for_() && Vectorizable(cexpr, {4, 8}, &vector_width)) {
        to_vectorize_ = true;
        Visit(cexpr.As<ir::For>(), &cexpr);
        CHECK(cexpr.is_block());
        to_vectorize_ = false;
      } else {
        Visit(&cexpr, &cexpr);
      }
    }

    reatch_vectorize_mark_ = false;
  }

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

  //! Determines whether to vectorize this block
  bool Vectorizable(const Expr &expr, const std::set<int> &vectorize_widths, int *vector_width) {
    LOG_INDENT(6);
    CHECK(expr.is_for_());
    CINN_DEBUG(2) << "checking for\n" << expr;
    auto &for_ = *expr.As<ir::For>();
    // check for's init, cond and extent
    auto *init_val = for_.iter_init.As<ir::IntImm>();
    auto *cond_le = for_.iter_cond.As<ir::LE>();
    auto *inc_val = for_.iter_inc.As<ir::IntImm>();

    if (!init_val && init_val->val() != 0) {
      CINN_DEBUG(3) << "init is not IntImm or value != 1, " << for_.iter_init;
      return false;
    }
    if (!cond_le) {
      CINN_DEBUG(3) << "cond_le fail, cond:" << for_.iter_cond;
      return false;
    }

    if (!cond_le->b.is_int_imm()) {
      CINN_DEBUG(3) << "cond_le value is not IntImm, " << cond_le->b;
      return false;
    }
    if (!inc_val || inc_val->val() != 1) {
      CINN_DEBUG(3) << "for iter_inc != 1, " << for_.iter_inc;
      return false;
    }

    int cond_extent = cond_le->b.As<ir::IntImm>()->val();
    int num_elements = cond_extent - init_val->val() + 1;
    if (!vectorize_widths.count(num_elements)) {
      CINN_DEBUG(3) << "num_elements not in vector_width set, " << num_elements;
      return false;
    }

    // check the block content
    auto *for_block = for_.body.As<ir::Block>();
    // All the expressions in the block should be vectorizable.
    // NOTE here is just a naive implementation.
    // TODO(Superjomn) enhance here.
    for (auto &expr : for_block->body) {
      if (expr.is_block() || expr.is_for_() || expr.is_if_then_else()) {
        CINN_DEBUG(3) << "found no simple expression:\n" << expr;
        return false;
      }
    }

    if (!ir::CollectExprNode<ir::SIMDOpr>(expr).empty()) {
      CINN_DEBUG(3) << "fail, already have SIMD opr";
      return false;
    }

    *vector_width = num_elements;
    return true;
  }

 private:
  bool to_vectorize_{false};
  bool reatch_vectorize_mark_{false};
  bool inside_reference_{false};
  bool is_block_subsequent_vectorize_for_{false};
};

class VectorizePass : public Pass<ir::Expr> {
 public:
  explicit VectorizePass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    VectorizeMutator vectorize_mutator;
    vectorize_mutator.Visit(expr, expr);

    ir::IRSimplify(expr);
  }
};

}  // namespace cinn

REGISTER_IR_PASS(vectorize, cinn::VectorizePass);
