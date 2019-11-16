#pragma once
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

class Builder {
 public:
  //! Build a network.
  ir::Expr Build(Session* session, Network&& net);

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
};

}  // namespace hlir
}  // namespace cinn
