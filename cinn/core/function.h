#include <string>
#include "cinn/core/buffer.h"
#include "cinn/ir/ir.h"

namespace cinn {
using ir::Expr;

struct Stage;
/**
 * Function is the core part of CINN.
 *
 * Definition of a Function:
 *
 *     Function func({inputs}, {outputs})
 *
 * We make both the inputs and outputs a list, so that if multiple exprs relays on the same temporarily values, they
 * can be composed in a same function.
 *
 * Call a Function:
 *
 *     Expr A, B, C;
 *     std::vector<Expr> outs = func({B, C});
 *
 * Usage:
 *
 *     Var i("i"),j("j");
 *     Expr A, B, C;
 *     A(i,j) = B(i,j) + C(i,j);
 *
 *     Function func0({B,C}, {A});
 *     func0.set_inline();
 *     // func0.dims(), get dimension of the cumputation.
 *
 *     Expr B0, C0;
 *     Expr A0 = func0(B0,C0);
 *     // will expand to
 *     Expr A0;
 *     A0(i,j) = B(i,j) + C(i,j)
 */
struct Function : public ir::ExprNode<Function> {
  //! Name of the function, should be unique across the compile context.
  std::string name;

  //! Pass argument by value.
  std::vector<Buffer *> arguments;

  //! For inline function to expand the definition inplace.
  std::vector<ir::Expr> argument_exprs;

  //! Body of the function.
  std::vector<Expr> body;
  // std::vector<Computation*> body;

  //! Define a function.
  static Expr make(const std::string &name, std::vector<Expr> inputs, std::vector<Expr> outputs);
};

}  // namespace cinn
