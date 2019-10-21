#pragma once
#include <memory>
#include <string>
#include <vector>
#include "cinn/hlir/operator.h"

namespace cinn {
namespace hlir {

/**
 * Program helps to hold a sequence of Operators.
 */
class Program {
 public:
  void AddOp(std::unique_ptr<Operator>&& op) { ops_.emplace_back(std::move(op)); }

  std::set<std::string> Inputs() const {
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

  std::set<std::string> Outputs() const {
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

  const std::vector<std::unique_ptr<Operator>>& ops() const { return ops_; }

  size_t size() const { return ops_.size(); }

 private:
  std::vector<std::unique_ptr<Operator>> ops_;
};

}  // namespace hlir
}  // namespace cinn
