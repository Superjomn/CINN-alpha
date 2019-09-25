#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/logging.h"

namespace cinn {

struct Mutator : public ir::IRMutator {
  void Mutate(ir::Expr *op, ir::Expr *expr) { ir::IRMutator::Mutate(op, expr); }

  void Mutate(ir::Reference *op, ir::Expr *expr) override {
    CHECK(op->target.is_tensor());
    auto *tensor = op->target.As<ir::Tensor>();

    ir::Expr new_iterator = op->iterators.front();
    for (size_t i = 1; i < op->iterators.size(); i++) {
      new_iterator = new_iterator * tensor->dims()[i - 1];
      new_iterator = new_iterator + op->iterators[i];
    }

    // replace the iterator.
    op->iterators.clear();
    op->iterators.push_back(new_iterator);
  }
};

class IndicesToAbsoluteOffsetPass : public Pass {
 public:
  explicit IndicesToAbsoluteOffsetPass(const std::string &name) : Pass(name) {}

  void Impl(ir::Expr *expr) override {
    Mutator mutator;
    mutator.Mutate(expr, expr);
  }
};

}  // namespace cinn

REGISTER_PASS(indices_to_absolute_offset, cinn::IndicesToAbsoluteOffsetPass);
