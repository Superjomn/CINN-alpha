#pragma once

#include <string>
#include <vector>
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/target.h"

namespace cinn {

/**
 * Module the the basic module of CINN program. It holds all the global buffers, functions and IR in a module.
 * A module corresponds to a C++ file or LLVM module.
 */
class Module {
 public:
  Module(const std::string& name, const Target& target) : name_(name), target_(target) {}

  const std::string& name() const { return name_; }
  const Target& target() const { return target_; }

  //! Add a function definition to the module.
  void AddFunction(const Expr& function);

  //! Fill the lower function IR, such as Allocate.
  void Lower();

  //! Dump some human-readable code.
  void Dump();

  // Link multiple modules together into one.
  static Module LinkModules(const std::string& name, const std::vector<Module>& modules);

 private:
  //! Lower a single function.
  // This method will analysis the data dependency, and allocate the temporary buffer.
  void LowerFunction(Expr& function);

  //! Process buffer's data type inference inside a context of function.
  static void BufferDTypeInference(Function* function);

  //! Process buffer's size inference inside a context of function.
  static void BufferSizeInference(Function* function);

 private:
  std::string name_;
  Target target_;
  std::vector<Expr> functions_;
};

}  // namespace cinn
