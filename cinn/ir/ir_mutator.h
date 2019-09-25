/**
 * IRMutator helps to traverse the expression tree and mutate some nodes. It is similar to the IRVisitor.
 */
#pragma once
#include "cinn/utils/macros.h"

namespace cinn {
struct Function;
namespace ir {

// Forward declare all the IR types.
#define __(op__) struct op__;
OP_ALL_WITHOUT_FUNCTION_FOR_EACH(__);
#undef __
struct Constant;

struct Expr;

class IRMutator {
 public:
  void Mutate(Expr* op, Expr* expr);

  virtual void Mutate(Constant* op, Expr* expr);

#define __(op__) virtual void Mutate(op__* op, Expr* expr);
  OP_ALL_FOR_EACH(__);
#undef __
};

}  // namespace ir
}  // namespace cinn
