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

  Expr CreateGlobalVars(Session* session, const Network& net) {
    std::vector<ir::Expr> exprs(
        {CreateExprForWeightDeclaration(*session, net), CreateExprForInputOutputDeclaration(*session, net)});
    return ir::Block::make(std::move(exprs));
  }

 private:
  Expr CreateExprForWeightDeclaration(const Session& session, const Network& network);
  Expr CreateExprForInputOutputDeclaration(const Session& session, const Network& network);
};

}  // namespace hlir
}  // namespace cinn
