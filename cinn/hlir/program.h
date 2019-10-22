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

  /**
   * The input tensors of this program.
   */
  std::set<std::string> Inputs() const;

  /**
   * The output tensors of this program.
   */
  std::set<std::string> Outputs() const;

  const std::vector<std::unique_ptr<Operator>>& ops() const { return ops_; }

  size_t size() const { return ops_.size(); }

 private:
  std::vector<std::unique_ptr<Operator>> ops_;
};

}  // namespace hlir
}  // namespace cinn
