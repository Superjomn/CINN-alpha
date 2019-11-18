#pragma once

#include <string>
#include <vector>
#include "cinn/core/cinn_context.h"
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
      : target_(target),
        size_(size),
        ptype_(ptype),
        name_(name.empty() ? GlobalContext().name_generator().NewBuffer() : name) {}

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

}  // namespace cinn
