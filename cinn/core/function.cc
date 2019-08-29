
#include "function.h"

namespace cinn {

Expr cinn::Function::make(const std::string &name, std::vector<Expr> inputs, std::vector<Expr> outputs) {
  auto node = std::make_shared<Function>();
  node->name = name;
  node->argument_exprs = inputs;
  node->body = outputs;
  return Expr(node);
}

}  // namespace cinn