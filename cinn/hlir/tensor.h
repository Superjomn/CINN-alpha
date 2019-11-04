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

  /**
   * Get the shape of the tensor.
   * @return the shape of the tensor.
   */
  const shape_t& shape() const { return shape_; }

  /**
   * Set the shape of the tensor.
   * @param x the shape
   */
  void set_shape(const shape_t& x);

  /**
   * Set the iterators manually, be sure that the iterators empty.
   * @param iterators
   */
  void set_iterators(const std::vector<ir::Expr>& iterators) const { iterators_ = iterators; }

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
  const ir::Expr& expr() const;

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

  void set_name(const std::string& x) { name_ = x; }
  const std::string& name() const { return name_; }

  /**
   * Get the inner representation in IR of this tensor, it is the unique identifier for this tensor in the isl
   * representation.
   */
  const std::string& ir_inner_name() const;

  std::string __repr__() const;

 private:
  void InitExpr();

  shape_t shape_;
  mutable ir::Expr expr_;
  mutable std::vector<ir::Expr> iterators_;
  std::vector<Stage> stages_;
  std::shared_ptr<Buffer> buffer_;
  //! Represented inside the IR.
  std::string ir_inner_name_;
  //! the name of this tensor in HLIR graph.
  std::string name_;
};

}  // namespace hlir
}  // namespace cinn
