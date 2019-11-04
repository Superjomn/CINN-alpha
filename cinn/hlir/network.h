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

class Network {
 public:
  Network(const std::string& name, Session* session) : name_(name), session_(session) {}

  /**
   * Add a MatMul operator.
   * @param x Name of the first input.
   * @param y Name of the second input.
   * @param out Name of the output.
   */
  void AddMatMul(const std::string& x, const std::string& y, const std::string& out);

  /**
   * Add a Tanh operator.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  void AddTanh(const std::string& x, const std::string& out);

  /**
   * Add a Sigmoid operator.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  void AddSigmoid(const std::string& x, const std::string& out);

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
  void AddElementwise(ElementwiseOpKind kind, const std::string& x, const std::string& y, const std::string& out);

  /**
   * Add a Reshape operator.
   * @param shape The new shape.
   * @param x Name of the input.
   * @param out Name of the output.
   */
  void AddReshape(const std::vector<int>& shape, const std::string& x, const std::string& out);

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
