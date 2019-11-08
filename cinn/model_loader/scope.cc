#include "cinn/model_loader/scope.h"

namespace cinn {
namespace model_loader {

Scope::~Scope() {
  for (auto *x : kids_) {
    if (x) {
      delete x;
    }
  }
}

Scope &Scope::NewScope() const {
  kids_.push_back(new Scope);
  kids_.back()->parent_ = this;
  return *kids_.back();
}

Variable *Scope::Var(const std::string &name) {
  auto *var = FindVar(name);
  if (var) return var;

  // create a new variable.
  vars_.emplace(name, std::unique_ptr<Variable>(new Variable));
  return vars_[name].get();
}

Variable *Scope::FindVar(const std::string &name) const {
  Variable *var{nullptr};
  var = FindLocalVar(name);
  const Scope *cur_scope = this;
  while (!var && cur_scope->parent()) {
    cur_scope = cur_scope->parent();
    var = cur_scope->FindLocalVar(name);
  }

  return var;
}

Variable *Scope::FindLocalVar(const std::string &name) const {
  auto it = vars_.find(name);
  if (it != vars_.end()) {
    return it->second.get();
  }
  return nullptr;
}

std::vector<std::string> Scope::LocalVarNames() const {
  std::vector<std::string> keys;
  for (const auto &item : vars_) {
    keys.push_back(item.first);
  }
  return keys;
}

}  // namespace model_loader
}  // namespace cinn
