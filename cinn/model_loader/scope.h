#pragma once

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cinn/model_loader/tensor.h"
#include "cinn/model_loader/variable.h"

namespace cinn {
namespace model_loader {

class Scope final {
 public:
  Scope() = default;
  // delete below two functions to allow pybind to recognise it cannot make a
  // copy
  // link:
  // https://stackoverflow.com/questions/53807248/pybind11-returning-a-pointer-to-a-container-of-unique-ptr
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  ~Scope();

  Scope& NewScope() const;

  Variable* Var(const std::string& name);

  Variable* FindVar(const std::string& name) const;

  Variable* FindLocalVar(const std::string& name) const;

  const Scope* parent() const { return parent_; }

  // Following the legacy scope interface.
  std::vector<std::string> LocalVarNames() const;

  /// ------------------------------------- helper functions for Tensor
  /// ----------------------------------
  // Create a Tensor variable. This will create a new Variable called `name`.
  Tensor* NewTensor(const std::string& name) {
    auto* var = Var(name);
    return var->GetMutable<Tensor>();
  }

  const Tensor* FindTensor(const std::string& name) {
    auto* var = FindVar(name);
    if (!var) return nullptr;
    return &var->Get<Tensor>();
  }

  Tensor* FindMutableTensor(const std::string& name) {
    auto* var = FindVar(name);
    if (!var) return nullptr;
    return var->GetMutable<Tensor>();
  }

 private:
  // Scope in `kids_` are owned by this class.
  mutable std::list<Scope*> kids_;
  const Scope* parent_{nullptr};
  std::unordered_map<std::string, std::unique_ptr<Variable>> vars_;
};

}  // namespace model_loader
}  // namespace cinn
