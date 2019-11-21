/**
 * Operator is the basic node of graph in hlir.
 */
#pragma once

#include <map>
#include <string>
#include "cinn/hlir/hlir_util.h"
#include "cinn/hlir/session.h"
#include "cinn/hlir/tensor.h"
#include "cinn/utils/any.h"

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
  /**
   * Construct an Operator.
   * @param type the type of the operator, such as "tanh", "fc".
   * @param layer the layer in the HLIR.
   * @param session the session that contains all the tensor in the whole program.
   */
  Operator(const std::string& type, HlirLayer layer, Session* session)
      : type_(type), layer_(layer), session_(session) {}

  void set_session(Session* x);

  /**
   * Compile to lower layer of operators or IR.
   */
  void Compile() {
    CHECK(!compiled_) << "operator duplicate compiled";
    InferenceOutputType();
    Resize();
    CompileImpl();
    compiled_ = true;
  }

  /**
   * Tell whether this operator is compiled, each operator can only compiled once.
   */
  bool compiled() const { return compiled_; }

  /**
   * Set a input argument.
   * @param argument the argument name.
   * @param value the name of the value tensor.
   */
  void SetInput(const std::string& argument, const std::string& value);

  /**
   * Get a input argument.
   * @param argument
   * @return the corresponding tensor.
   */
  const Tensor* GetInput(const std::string& argument) const;

  /**
   * Set a output argument.
   * @param argument
   * @param value
   */
  void SetOutput(const std::string& argument, const std::string& value) {
    CHECK(!value.empty()) << "value is null";
    output_argument2value_[argument] = value;
  }

  /**
   * Get a output argument.
   * @param argument
   * @return
   */
  Tensor& GetOutput(const std::string& argument);

  const std::map<std::string, std::string>& inputs() const { return input_argument2value_; }
  const std::map<std::string, std::string>& outputs() const { return output_argument2value_; }

  const std::string& type() const { return type_; }

  template <typename T>
  T& param() {
    return *param_.get_mutable<T>();
  }
  template <typename T>
  const T& param() const {
    return param_.get<T>();
  }

 protected:
  /**
   * Inference output's type.
   */
  virtual void InferenceOutputType() = 0;

  /**
   * Resize the output's shape.
   */
  virtual void Resize() = 0;
  /**
   * The main compute logic that one should implement.
   */
  virtual void CompileImpl() = 0;

  std::string type_;
  HlirLayer layer_{HlirLayer::kUnk};
  Session* session_;
  std::map<std::string, std::string> input_argument2value_;
  std::map<std::string, std::string> output_argument2value_;
  Any param_;
  bool compiled_ = false;
};

}  // namespace hlir
}  // namespace cinn
