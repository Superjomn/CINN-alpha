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

    int init_value;
    if (!ir::IsConstantFor(expr, vector_width, &init_value)) return false;
    if (init_value != 0) return false;

    if (!ir::CollectExprNode<ir::SIMDOpr>(expr).empty()) {
      CINN_DEBUG(3) << "fail, already have SIMD opr";
      return false;
    }

    return true;
  }

 private:
  bool to_vectorize_{false};
  bool reatch_vectorize_mark_{false};
  bool inside_reference_{false};
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
