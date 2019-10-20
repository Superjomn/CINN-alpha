#pragma once
#include <utility>
#include <vector>
#include "cinn/core/stage.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {

using shape_t = std::vector<int>;

/**
 * Tensor is the variables of the program.
 */
class Tensor {
 public:
  ir::Expr Elem() const {
    ir::Expr x = expr();
    for (int i = 0; i < shape_.size(); i++) {
      x = x[iterators()[i]];
    }
    return x;
  }

  const shape_t& shape() const { return shape_; }
  void set_shape(const shape_t& x) { shape_ = x; }

  const std::vector<ir::Expr>& iterators() const {
    CHECK(!shape_.empty());
    if (iterators_.empty()) {
      iterators_.resize(shape_.size(), ir::Var());
    }
    return iterators_;
  }
  std::vector<ir::Expr>& iterators() {
    CHECK(!shape_.empty());
    if (iterators_.empty()) {
      iterators_.resize(shape_.size(), ir::Var());
    }
    return iterators_;
  }

  const ir::Expr& expr() const { return expr_; }
  ir::Expr& expr() { return expr_; }

  void AddStage(Stage&& stage) { stages_.emplace_back(std::move(stage)); }
  Stage& last_stage() { return stages_.back(); }

 private:
  shape_t shape_;
  ir::Expr expr_;
  mutable std::vector<ir::Expr> iterators_;
  std::vector<Stage> stages_;
};

}  // namespace hlir
}  // namespace cinn
