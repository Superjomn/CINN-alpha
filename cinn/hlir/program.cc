#include "cinn/hlir/program.h"

namespace cinn {
namespace hlir {

std::set<std::string> Program::Inputs() const {
  std::set<std::string> vars;

  for (auto& op : ops_) {
    for (auto& item : op->inputs()) {
      vars.insert(item.second);
    }
  }

  for (auto& op : ops_) {
    for (auto& item : op->outputs()) {
      if (vars.count(item.second)) vars.erase(item.second);
    }
  }
  return vars;
}

std::set<std::string> Program::Outputs() const {
  std::set<std::string> vars;

  for (auto& op : ops_) {
    for (auto& item : op->outputs()) {
      vars.insert(item.second);
    }
  }

  for (auto& op : ops_) {
    for (auto& item : op->inputs()) {
      if (vars.count(item.second)) vars.erase(item.second);
    }
  }
  return vars;
}

}  // namespace hlir
}  // namespace cinn