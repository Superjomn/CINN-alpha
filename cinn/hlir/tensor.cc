#include "cinn/hlir/tensor.h"
#include "cinn/ir/ir.h"

namespace cinn {
namespace hlir {

ir::Expr Tensor::Elem() const {
  CHECK(!shape().empty());
  CHECK_EQ(iterators().size(), shape().size());
  auto node = ir::Reference::make(expr(), iterators());
  node.InferenceIteratorDomain();
  return node;
}

const std::vector<ir::Expr> &Tensor::iterators() const {
  CHECK(!shape_.empty());
  if (iterators_.empty()) {
    for (int i = 0; i < shape().size(); i++) {
      iterators_.emplace_back(ir::Var());
    }
  } else {
    CHECK_EQ(shape().size(), iterators_.size());
  }
  return iterators_;
}

std::vector<ir::Expr> &Tensor::iterators() {
  CHECK(!shape_.empty());
  if (iterators_.empty()) {
    for (int i = 0; i < shape().size(); i++) {
      iterators_.emplace_back(ir::Var());
    }
  } else {
    CHECK_EQ(shape().size(), iterators_.size());
  }
  return iterators_;
}

ir::Expr &Tensor::expr() {
  if (!expr_.valid()) {
    std::vector<ir::Constant> ir_shape;
    for (int v : shape()) {
      ir_shape.emplace_back(v);
    }
    expr_ = ir::Expr(ir_shape, primitive_t::float32);
  }
  return expr_;
}

const ir::Expr &Tensor::expr() const {
  if (!expr_.valid()) {
    std::vector<ir::Constant> ir_shape;
    for (int v : shape()) {
      ir_shape.emplace_back(v);
    }
    expr_ = ir::Expr(ir_shape, primitive_t::float32);
  }
  return expr_;
}

}  // namespace hlir
}  // namespace cinn