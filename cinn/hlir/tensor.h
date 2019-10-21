#pragma once
#include <utility>
#include <vector>
#include "cinn/core/stage.h"
#include "cinn/hlir/buffer.h"
#include "cinn/ir/ir.h"
#include "cinn/utils/name_generator.h"

namespace cinn {
namespace hlir {

using shape_t = std::vector<int>;

/**
 * Tensor represents the in the Graph(SSA), it helps to record the operations on the variable.
 *
 * e.g. A program snippet:
 *
 * %5 = Scale(%2, 1.2f);
 * %6 = AddScalar(%5, 1.f);
 * %7 = Add(%6, %5);
 *
 * The SSA variables %5,%6,%7 are all Tensors, each holds a vector of Stages to record the operations on them.
 *
 * %5: { %5[i,j] = %2[i,j] + 1.2f }
 * %6: { %6[i,j] = %5[i,j] + 1.f; }
 * %7: { %7[i,j] = %6[i,j] + %5[i,j] }
 *
 */
class Tensor {
 public:
  Tensor() { buffer_ = std::make_shared<Buffer>(); }

  /**
   * Get the ir::Expr of the element in the tensor(such as A[i][j]).
   * @return the Expr.
   */
  ir::Expr Elem() const;

  const shape_t& shape() const { return shape_; }
  void set_shape(const shape_t& x) { shape_ = x; }

  void set_iterators(const std::vector<ir::Expr>& iterators) {
    CHECK(iterators_.empty());
    iterators_ = iterators;
  }

  /**
   * Get the iterator Vars of this tensor.
   * @return a vector of Exprs representing the iterators in order.
   */
  const std::vector<ir::Expr>& iterators() const;
  /**
   *
   * @return the mutable iterators.
   */
  std::vector<ir::Expr>& iterators();

  /**
   * Get the Expr of this Tensor, should be a Tensor declaration such as `Expr("A", primitive_t::fp32, {20, 20})`.
   * @return the corresponding Expr.
   */
  const ir::Expr& expr() const {
    if (!expr_.valid()) {
      std::vector<ir::Constant> ir_shape;
      for (int v : shape()) {
        ir_shape.emplace_back(v);
      }
      expr_ = ir::Expr(ir_shape, primitive_t::float32);
    }
    return expr_;
  }

  /**
   * Get the Expr of this Tensor, should be a Tensor declaration such as `Expr("A", primitive_t::fp32, {20, 20})`.
   * @return the corresponding Expr.
   */
  ir::Expr& expr();

  /**
   * Share the underlying memory buffer with another Tensor.
   * @param other the other Tensor.
   */
  void ShareBufferWith(Tensor* other) const {
    CHECK(buffer_);
    other->buffer_ = buffer_;
  }

  /**
   * Add a stage to the Tensor.
   * @param stage the stage to add.
   */
  Stage& AddStage(Stage&& stage) {
    stages_.emplace_back(std::move(stage));
    return stages_.back();
  }

  const std::vector<Stage>& stages() { return stages_; }

  /**
   * The latest stage added.
   * @return the corresponding stage.
   */
  Stage& last_stage() { return stages_.back(); }

 private:
  shape_t shape_;
  mutable ir::Expr expr_;
  mutable std::vector<ir::Expr> iterators_;
  std::vector<Stage> stages_;
  std::shared_ptr<Buffer> buffer_;
};

}  // namespace hlir
}  // namespace cinn
