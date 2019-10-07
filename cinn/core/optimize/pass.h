#pragma once
#include <string>
#include "cinn/ir/ir.h"

/*
 * The Pass is the base class of all the passes.
 */

namespace cinn {

/**
 * @brief Pass is the basic module of optimizer. One can implement the Impl method to create a new pass.
 */
class Pass {
  std::string name_;

 public:
  explicit Pass(const std::string& name) : name_(name) {}

  void Run(ir::Expr* expr) {
    LOG(INFO) << "Running " << name_ << " pass";
    Impl(expr);
  }

  //! The implementation of this pass.
  virtual void Impl(ir::Expr* expr) = 0;

  //! The name of this pass.
  const std::string& name() const { return name_; }

  virtual ~Pass() = default;
};

}  // namespace cinn
