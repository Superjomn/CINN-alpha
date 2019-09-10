#include "cinn/ir/ir_helper.h"
#include <memory>
#include "cinn/utils/macros.h"

namespace cinn {
namespace ir {

#define TWO_PARAM_OP(op__)                \
  case NodeTy::op__: {                    \
    auto* add = expr.As<op__>();          \
    auto node = std::make_shared<op__>(); \
    node->a = CopyExpr(add->a);           \
    node->b = CopyExpr(add->b);           \
    return Expr(node);                    \
  }

#define ONE_PARAM_OP(op__)                \
  case NodeTy::op__: {                    \
    auto* x = expr.As<op__>();            \
    auto node = std::make_shared<op__>(); \
    node->a = CopyExpr(x->a);             \
    return Expr(node);                    \
  }

Expr CopyExpr(const Expr& expr) {
  switch (expr.type()) {
    OP_2_ARGS_FOR_EACH(TWO_PARAM_OP);
    OP_1_ARGS_FOR_EACH(ONE_PARAM_OP);
    case NodeTy::Call: {
      auto* x = expr.As<Call>();
      auto node = std::make_shared<Call>();
      for (auto& arg : x->arguments) {
        node->arguments.push_back(CopyExpr(arg));
      }
      node->caller = x->caller;
      return Expr(node);
    }
    case NodeTy::Reference: {
      auto* x = expr.As<Reference>();
      auto node = std::make_shared<Reference>();
      for (auto& iterator : x->iterators) {
        node->iterators.push_back(iterator);  // copied
      }
      node->target = CopyExpr(x->target);
      return Expr(node);
    }
    case NodeTy::Var: {
      auto* x = expr.As<Var>();
      auto node = std::make_shared<Var>();
      *node = *x;
      return Expr(node);
    }
    case NodeTy::Int: {
      auto* x = expr.As<IntImm>();
      auto node = std::make_shared<IntImm>();
      *node = *x;
      return Expr(node);
    }

    default:
      LOG(FATAL) << "unsupported type " << static_cast<int>(expr.type());
  }
}

#undef ONE_PARAM_OP
#undef TWO_PARAM_OP

}  // namespace ir
}  // namespace cinn