#pragma once

#include <string>
#include <vector>
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/type.h"

namespace cinn {

// Buffer is used to store the result of computation and represent the argument of function.
class Buffer : public ir::Expr {
  bool allocated_{false};

  // Dimensions of this buffer.
  std::vector<ir::Expr> dims_;

  // Name of this buffer, should be unique.
  std::string name_;

  primitive_t ptype_{primitive_t::unk};

  memory_location_t memory_location_;

  argument_t arg_type_{argument_t::unknown};

 public:
  using dims_t = std::vector<Expr>;

  //! Construct the buffer, the primitive type will inference from the attached expression.
  Buffer(const std::string& name);
  //! Construct the buffer with explicit dimensions and primitive type.
  Buffer(const std::string& name, dims_t dims, primitive_t type) : name_(name), dims_(dims), ptype_(type) {}
  //! Bind to an expression, the dimensions will be inferenced.
  void Bind(Expr expr) {
    CHECK(expr.is_reference()) << "The buffer can only bind to Reference expression";
    // Inference the shape.
    auto* reference = expr.As<ir::Reference>();
  }
  //! Size of the buffer.
  Expr size() const;
  //! Get the dimensions of the buffer.
  const dims_t& dims() const { return dims_; }
  //! Get the primitive tyep of the buffer.
  primitive_t ptype() const { return ptype_; }
};

}  // namespace cinn
