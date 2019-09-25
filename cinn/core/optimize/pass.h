#pragma once
#include <string>
#include "cinn/ir/ir.h"

/*
 * The Pass is the base class of all the passes.
 */

namespace cinn {

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

  const std::string& name() const { return name_; }
};

}  // namespace cinn
