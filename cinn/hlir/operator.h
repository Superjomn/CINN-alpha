/**
 * Operator is the basic node of graph in hlir.
 */
#pragma once

#include <map>
#include <string>
#include "cinn/hlir/hlir_util.h"
#include "cinn/hlir/session.h"
#include "cinn/hlir/tensor.h"

namespace cinn {
namespace hlir {

/**
 * Operator is the basic node of HLIR graph. It has several levels of implementation, the current layers are
 * - model-wise: loaded directly from the model,
 * - mathlib-wise: implemented by third-party math library,
 * - instruction-wise: will lower to CINN IR.
 */
class Operator {
 public:
  Operator(const std::string& type, HlirLayer layer, Session* session)
      : type_(type), layer_(layer), session_(session) {}

  /**
   * Compile to lower layer of operators or IR.
   */
  void Compile() { CompileImpl(); }

  void SetInput(const std::string& argument, const std::string& value) {
    CHECK(!argument.empty());
    CHECK(!value.empty());
    argument2value_[argument] = value;
  }

  const Tensor& GetInput(const std::string& argument) const {
    auto it = argument2value_.find(argument);
    CHECK(it != argument2value_.end());
    return *session_->GetTensor(it->second);
  }

  Tensor& GetOutput(const std::string& argument) {
    auto it = argument2value_.find(argument);
    CHECK(it != argument2value_.end());
    return *session_->GetTensor(it->second);
  }

 protected:
  virtual void CompileImpl() = 0;

  std::string type_;
  HlirLayer layer_{HlirLayer::kUnk};
  Session* session_;
  std::map<std::string, std::string> argument2value_;
};

}  // namespace hlir
}  // namespace cinn
