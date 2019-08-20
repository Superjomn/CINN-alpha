#pragma once

#include <string>
#include <vector>
#include "cinn/target.h"

namespace cinn {

class Module {
 public:
  Module(const std::string& name, const Target& target) : name_(name), target_(target) {}

  const std::string& name() const { return name_; }
  const Target& target() const { return target_; }

  // Link multiple modules together into one.
  static Module LinkModules(const std::string& name, const std::vector<Module>& modules);

 private:
  std::string name_;
  Target target_;
};

}  // namespace cinn
