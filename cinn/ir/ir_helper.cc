#include "cinn/ir/ir_helper.h"
#include <memory>
#include "cinn/ir/ir_visitor.h"
#include "cinn/utils/macros.h"

namespace cinn {
namespace ir {

#define TWO_PARAM_OP(op__)                \
  case NodeTy::op__: {                    \
    auto* add = expr.As<op__>();          \
    auto node = std::make_shared<op__>(); \
    node->a = CopyExpr(add->a);           \
    node->b = CopyExpr(add->b);           \
    node->set_ptype(add->ptype());        \
    return Expr(node);                    \
  }

#define ONE_PARAM_OP(op__)                \
  case NodeTy::op__: {                    \
    auto* x = expr.As<op__>();            \
    auto node = std::make_shared<op__>(); \
    node->a = CopyExpr(x->a);             \
    node->set_ptype(x->ptype());          \
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
      node->set_ptype(x->ptype());
      return Expr(node);
    }
    case NodeTy::Reference: {
      auto* x = expr.As<Reference>();
      auto node = std::make_shared<Reference>();
      for (auto& iterator : x->iterators) {
        node->iterators.push_back(iterator);  // copied
      }
      node->target = CopyExpr(x->target);
      node->set_ptype(x->ptype());
      return Expr(node);
    }
    case NodeTy::Tensor: {
      auto* x = expr.As<Tensor>();
      auto node = Tensor::make(x->dims(), x->ptype(), x->name());
      node.set_ptype(x->ptype());
      return Expr(node);
    }
    case NodeTy::Var: {
      auto* x = expr.As<Var>();
      auto node = std::make_shared<Var>();
      *node = *x;
      node->set_ptype(x->ptype());
      return Expr(node);
    }
    case NodeTy::IntImm: {
      auto* x = expr.As<IntImm>();
      auto node = std::make_shared<IntImm>();
      *node = *x;
      node->set_ptype(x->ptype());
      return Expr(node);
    }

    case NodeTy::FloatImm: {
      auto* x = expr.As<FloatImm>();
      auto node = std::make_shared<FloatImm>();
      *node = *x;
      node->set_ptype(x->ptype());
      return Expr(node);
    }

    case NodeTy ::Parameter: {
      auto* x = expr.As<Constant>();
      auto node = std::make_shared<Constant>();
      *node = *x;
      node->set_ptype(x->ptype());
      return Expr(node);
    }

    default:
      LOG(FATAL) << "unsupported type " << GetNodeTyRepr(expr.type());
  }
}

#undef ONE_PARAM_OP
#undef TWO_PARAM_OP

std::vector<const Var*> CollectVarsFromExpr(const Expr& expr) {
  class Visitor : public IRVisitor {
    int id_;

   public:
    std::set<const Var*> vars;

    void Visit(const Expr* op) override { IRVisitor::Visit(op); }
    void Visit(const Var* op) override { vars.insert(op); }
  };

  Visitor visitor;
  visitor.Visit(&expr);

  return std::vector<const Var*>(visitor.vars.begin(), visitor.vars.end());
}

template <>
std::vector<const Reference*> CollectExprNode<Reference>(const Expr& expr) {
  class Visitor : public IRVisitor {
   public:
    std::set<const Reference*> set;

    void Visit(const Expr* op) override { IRVisitor::Visit(op); }
    void Visit(const Reference* op) override {
      set.insert(op);
      IRVisitor::Visit(op);
    }
  };

  Visitor visitor;
  visitor.Visit(&expr);

  return std::vector<const Reference*>(visitor.set.begin(), visitor.set.end());
}

struct IREqualTeller : public IRVisitorBase<bool, const ir::Expr*> {
  bool Visit(const Expr* a, const Expr* b) override { return a == b || IRVisitorBase::Visit(a, b); }

#define OP_2PARAM(op__)                                            \
  bool Visit(const ir::op__* a, const Expr* expr) override {       \
    auto* b = expr->As<ir::op__>();                                \
    return a == b || (Visit(&a->a, &b->a) && Visit(&a->b, &b->b)); \
  }

#define OP_1PARAM(op__)                                      \
  bool Visit(const ir::op__* a, const Expr* expr) override { \
    auto* b = expr->As<ir::op__>();                          \
    return a == b || Visit(&a->a, &b->a);                    \
  }

  OP_2PARAM(Add);
  OP_2PARAM(Sub);
  OP_2PARAM(Mul);
  OP_2PARAM(Div);
  OP_2PARAM(Mod);
  OP_1PARAM(Minus);
  OP_2PARAM(EQ);
  OP_2PARAM(NE);
  OP_2PARAM(LT);
  OP_2PARAM(LE);
  OP_2PARAM(GT);
  OP_2PARAM(GE);
  OP_2PARAM(And);
  OP_2PARAM(Or);

  OP_2PARAM(Min);
  OP_2PARAM(Max);

  OP_2PARAM(Assign);
  OP_2PARAM(Let);
  OP_2PARAM(IncreAssign);

  OP_1PARAM(Exp);
  OP_1PARAM(Tanh);
  OP_1PARAM(Sigmoid);

#undef OP_2PARAM
#undef OP_1PARAM

  bool Visit(const ir::Var* a, const ir::Expr* expr) override {
    const auto* b = expr->As<ir::Var>();
    return a == b || a->name() == b->name();
  }
  bool Visit(const ir::Constant* a, const ir::Expr* expr) override {
    const auto* b = expr->As<ir::Constant>();
    if (a == b) return true;
    return *a == *b;
  }

  bool Visit(const ir::Param* op, const ir::Expr* expr) override {
    LOG(FATAL) << "not supported yet";
    return false;
  }

  bool Visit(const ir::Reference* a, const ir::Expr* expr) override {
    auto* b = expr->As<ir::Reference>();
    if (a == b) return true;

    if (!Visit(&a->target, &b->target)) return false;
    // NOTE vague here.
    if (a->iterators.size() != b->iterators.size()) return false;
    for (int i = 0; i < a->iterators.size(); i++) {
      if (!Visit(&a->iterators[i], &b->iterators[i])) return false;
    }
    return true;
  }

  bool Visit(const Function* a, const ir::Expr* expr) override {
    auto* b = expr->As<Function>();

    if (a == b) return true;
    if (a->inputs().size() != b->inputs().size()) return false;
    if (a->outputs().size() != b->outputs().size()) return false;

    for (int i = 0; i < a->inputs().size(); i++) {
      if (!Visit(&a->inputs()[i], &b->inputs()[i])) return false;
    }
    for (int i = 0; i < a->outputs().size(); i++) {
      if (!Visit(&a->outputs()[i], &b->outputs()[i])) return false;
    }
    return Visit(&a->ComputeTransformedExpr(), &b->ComputeTransformedExpr());
  }

  bool Visit(const For* a, const Expr* expr) override {
    auto* b = expr->As<For>();
    if (a == b) return true;
    // check Var.
    // NOTE here is vague.
    if (a->iterator.name() != b->iterator.name()) return false;
    if (!Visit(&a->iter_init, &b->iter_init)) return false;
    if (!Visit(&a->iter_cond, &b->iter_cond)) return false;
    return Visit(&a->body, &b->body);
  }

  bool Visit(const IfThenElse* a, const Expr* expr) override {
    auto* b = expr->As<IfThenElse>();
    if (a == b) return true;

    if (!Visit(&a->condition, &b->condition)) return false;
    CHECK(a->true_block.valid());
    CHECK(b->true_block.valid());
    if (!Visit(&a->true_block, &b->true_block)) return false;
    if (a->false_block.valid() || b->false_block.valid()) {
      if (!a->false_block.valid() || !b->false_block.valid()) return false;
      return Visit(&a->false_block, &b->false_block);
    }
    return true;
  }

  bool Visit(const Block* a, const Expr* expr) override {
    auto* b = expr->As<Block>();
    if (a == b) return true;

    if (a->exprs.size() != b->exprs.size()) return false;
    for (int i = 0; i < a->exprs.size(); i++) {
      if (!Visit(&a->exprs[i], &b->exprs[i])) return false;
    }
    return true;
  }

  bool Visit(const Call* a, const Expr* expr) override {
    auto* b = expr->As<Call>();
    if (a == b) return true;

    if (a->arguments.size() != b->arguments.size()) return false;
    for (int i = 0; i < a->arguments.size(); i++) {
      if (!Visit(&a->arguments[i], &b->arguments[i])) return false;
    }
    return true;
  }

  bool Visit(const Stmt* a, const Expr* expr) override { LOG(FATAL) << "not supported yet"; }
  bool Visit(const Tensor* a, const Expr* expr) override { LOG(FATAL) << "not supported yet"; }
  bool Visit(const Statement* a, const Expr* expr) override { LOG(FATAL) << "not supported yet"; }
  bool Visit(const Allocate* a, const Expr* expr) override { LOG(FATAL) << "not supported yet"; }

  bool Visit(const IntImm* a, const Expr* expr) override {
    auto* b = expr->As<IntImm>();
    if (a == b) return true;
    return a->val() == b->val();
  }

  bool Visit(const FloatImm* a, const Expr* expr) override {
    auto* b = expr->As<FloatImm>();
    if (a == b) return true;
    return a->val() == b->val();
  }
};

bool IREquals(const Expr& a, const Expr& b) {
  if (a.ptr() == b.ptr()) return true;
  IREqualTeller teller;
  return teller.Visit(&a, &b);
}

}  // namespace ir
}  // namespace cinn
