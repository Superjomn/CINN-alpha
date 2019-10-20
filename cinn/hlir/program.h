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
  const std::shared_ptr<Operator>& NewOp(const std::string& type,
                                         HlirLayer layer,
                                         const std::vector<std::string>& inputs,
                                         const std::vector<std::string>& outputs) {
    return ops_.back();
  }

  const std::vector<std::shared_ptr<Operator>>& ops() const { return ops_; }

 private:
  std::vector<std::shared_ptr<Operator>> ops_;
};

}  // namespace hlir
}  // namespace cinn
