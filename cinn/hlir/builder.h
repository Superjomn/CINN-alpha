#pragma once
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

class Builder {
 public:
  //! Build a network.
  ir::Expr Build(Session* session, Network* net);

  /**
   * Transform an expression to C source code.
   * @param expr the expression
   * @prefix the prefix of the file to persist the content of the header and source file.
   */
  void ToCSourceCode(ir::Expr expr, const std::string& prefix);

 protected:
  /**
   * In CINN, declare all the buffers(as global variables).
   */
  Expr DeclBuffersGlobal(Session* session, const Network& net);

 private:
  Expr CreateExprForWeightDeclaration(const Session& session, const Network& network);
  Expr CreateExprForInputOutputDeclaration(const Session& session, const Network& network);

  /**
   * Create the functions for loadding inputs data.
   *
   * such as generating code like void load_input_x(float32_t* x_), copies data from x_ to buffer x.
   */
  Expr CreateLoadInputFns(const Network& net, const Session& session);

  /**
   * Create the functions for readding output data.
   *
   * For output x, it will generate code like void read_output_x(float32_t* y_);
   */
  Expr CreateGetOutputFns(const Network& net, const Session& session);

  /**
   * Create main_ function, the main_ function is used to run the model.
   *
   * The generated code is similar to
   *
   *    void main__();
   */
  Expr CreateMainFn(Expr expr);

  /**
   * Add the main function to the main program(append to the end).
   * @param program the main program.
   * @param main_fn the main_ function(expression of ir::Block)
   */
  void AddMainFnToProgram(Expr* program, Expr main_fn);

  /**
   * Add the input and output functions to the main program (insert to the head).
   * @param program the main program.
   * @param io_fns the input or output function block(expression of ir::Block).
   */
  void AddIOFnsToProgram(Expr* program, Expr io_fns);

  /**
   * Automatically fuse the stages with dependencies.
   */
  void AutoFuseStages(std::vector<Function>* fns);

  const char* main_fn_name = "main_";
  const char* load_fn_name_format = "set_input_%s";
  const char* read_fn_name_format = "get_output_%s";
};

}  // namespace hlir
}  // namespace cinn
