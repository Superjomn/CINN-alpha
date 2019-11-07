#pragma once

#include <string>
#include <vector>
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/type.h"

namespace cinn {

/**
 * Helper class to make a ir::BufferOpr easier.
 */
class Buffer {
  Target target_;
  ir::Expr size_;
  primitive_t ptype_;
  std::string name_;
  bool is_created_{false};
  bool is_destroyed_{false};
  int referenced_count_{};

 public:
  Buffer(Target target, ir::Expr size, primitive_t ptype, const std::string& name = "")
      : target_(target), size_(size), ptype_(ptype), name_(name.empty() ? NameGenerator::Global().NewBuffer() : name) {}

  //! Resize the buffer to size, we will grow the original buffer so that it can be shared in multiple usages.
  void Resize(ir::Expr size) { this->size_ = ir::Max_(this->size_, size); }

  //! The expression to create a buffer.
  ir::Expr create_expr() {
    CHECK(!is_destroyed_);
    CHECK(!is_created_);
    is_created_ = true;
    return ir::BufferOpr::make(target_, size_, ir::BufferOpr::Opr::kCreate, ptype_, name_);
  }
  //! The expression to destroy a buffer.
  ir::Expr destroy_expr() {
    CHECK(is_created_);
    CHECK(!is_destroyed_);
    is_destroyed_ = true;
    return ir::BufferOpr::make(target_, size_, ir::BufferOpr::Opr::kDestroy, ptype_, name_);
  }
  //! The expression to reference a buffer.
  ir::Expr reference() {
    CHECK(is_created_);
    CHECK(!is_destroyed_);
    referenced_count_++;
    return ir::BufferOpr::make(target_, size_, ir::BufferOpr::Opr::kReference, ptype_, name_);
  }

  int referenced_count() const { return referenced_count_; }
  const std::string& name() const { return name_; }
};

/*
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
 */

}  // namespace cinn
