#pragma once
/**
 * This file defines Network, the API to make model construction easier.
 */

#include <memory>
#include <string>
#include <vector>
#include "cinn/hlir/operator.h"
#include "cinn/hlir/program.h"
#include "cinn/hlir/session.h"

namespace cinn {
namespace hlir {

/**
 * Network is the high-level API.
 */
class Network {
 public:
  struct Var {
    Var(const std::string& name) : name(name) {}

    //! Tell whether the variale is valid.
    operator bool() { return !name.empty(); }

    std::string name;
  };

  Network(const std::string& name, Session* session) : name_(name), session_(session) {}

  //! Declare an input placeholder.
  Var DeclInput(const std::string& name, primitive_t ptype);
  //! Declare an output placeholder.
  Var DeclOutput(const std::string& name, primitive_t ptype);
  //! Declare a weight for the model.
  template <typename T>
  Var DeclWeight(const std::string& name, primitive_t ptype, const Shape& shape, const T* data) {
    Tensor* tensor = session_->NewTensor(name);
    auto buf = std::make_shared<Buffer>(name + "_buf", ptype);
    buf->Resize(shape.num_bytes(ptype));
    buf->SetData(data);
    tensor->AttachBuffer(buf);
    tensor->set_is_weight();
  }

  /**
   * Add a Fully Connected layer.
   * @param x the input.
   * @param w the weight.
   * @param b the bias, leave empty is not valid.
   * @return the output.
   */
  Var AddFc(Var x, Var w, Var b);

  /**
   * Add a MatMul operator.
   * @param x Name of the first input.
   * @param y Name of the second input.
   * @param out Name of the output.
   */
  Var AddMatMul(const Var& x, const Var& y);

  /**
   * Add a Tanh operator.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  Var AddTanh(Var x);

  /**
   * Add a Sigmoid operator.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  Var AddSigmoid(Var x);

  enum class ElementwiseOpKind {
    kAdd = 0,
    kSub,
    kMul,
    kDiv,
  };

  /**
   * Add an Elementwise operator.
   * @param kind The kind of this elementwise operation.
   * @param x Name of the first input.
   * @param y Name of the second input.
   * @param out Name of the output.
   */
  Var AddElementwise(ElementwiseOpKind kind, Var x, Var y);

  /**
   * Add a Reshape operator.
   * @param shape The new shape.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  Var AddReshape(const std::vector<int>& shape, Var x);

  /**
   * Compile the network and generate a program.
   *
   * NOTE Once it compiled, the network is invalid for further modification.
   */
  Program Compile();

  size_t num_operators() { return operators_.size(); }

 private:
  //! Name of this network.
  std::string name_;
  //! Session used for all the operators.
  Session* session_{};

  std::vector<std::unique_ptr<Operator>> operators_;
};

}  // namespace hlir
}  // namespace cinn
