#include "cinn/ir/ir_helper.h"
#include <ginac/ginac.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cinn/ir/ir_mutator.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ir_visitor.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/macros.h"
#include "cinn/utils/string.h"

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

    case NodeTy::Constant: {
      auto* x = expr.As<Constant>();
      auto node = std::make_shared<Constant>();
      *node = *x;
      node->set_ptype(x->ptype());
      return Expr(node);
    }

    case NodeTy::CallOnce: {
      auto* x = expr.As<CallOnce>();
      auto node = std::make_shared<CallOnce>();
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

#define __(type__)                                              \
  template <>                                                   \
  std::vector<Expr> CollectExprNode<type__>(const Expr& expr) { \
    class Mutator : public IRMutator {                          \
     public:                                                    \
      std::vector<Expr> exprs;                                  \
      std::set<std::string> keys;                               \
      void Visit(const Expr* op, Expr* expr) override {         \
        if (expr->type() == type__::node_type) {                \
          auto key = ir::Dump(*expr);                           \
          if (!keys.count(key)) {                               \
            exprs.push_back(*expr);                             \
            keys.insert(key);                                   \
          }                                                     \
        }                                                       \
        IRMutator::Visit(op, expr);                             \
      }                                                         \
    };                                                          \
                                                                \
    Mutator visitor;                                            \
    visitor.Visit(&expr, const_cast<ir::Expr*>(&expr));         \
                                                                \
    return visitor.exprs;                                       \
  }
__(SIMDOpr);
__(Reference);
__(Var);
__(Block);
__(Assign);
#undef __

struct IRCopy : public IRVisitorBase<void, ir::Expr*> {
  void Visit(const Expr* a, Expr* b) override { IRVisitorBase::Visit(a, b); }

#define OP_2PARAM(op__)                          \
  void Visit(const ir::op__* op, ir::Expr* to) { \
    ir::Expr a, b;                               \
    Visit(&op->a, &a);                           \
    Visit(&op->b, &b);                           \
    to->Reset(ir::op__::make(a, b));             \
  }
#define OP_1PARAM(op__)                          \
  void Visit(const ir::op__* op, ir::Expr* to) { \
    ir::Expr a;                                  \
    Visit(&op->a, &a);                           \
    to->Reset(ir::op__::make(a));                \
  }

  OP_2PARAM(Add);
  OP_2PARAM(Sub);
  OP_2PARAM(Mul);
  OP_2PARAM(Div);
  OP_2PARAM(Mod);
  OP_1PARAM(Minus);
  OP_1PARAM(Not);
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
  OP_2PARAM(SumAssign);
  OP_2PARAM(SubAssign);
  OP_2PARAM(MulAssign);
  OP_2PARAM(DivAssign);

  OP_1PARAM(Exp);
  OP_1PARAM(Tanh);
  OP_1PARAM(Sigmoid);

#undef OP_2PARAM
#undef OP_1PARAM

 protected:
  void Visit(const ir::Var* op, Expr* to) override {
    Var a(*op);
    *to = Expr(a);
  }
  void Visit(const Stmt* op, Expr* to) override { NOT_IMPLEMENT }
  void Visit(const For* op, Expr* to) override {
    Expr init, cond, inc, body;
    Visit(&op->iter_cond, &cond);
    Visit(&op->iter_inc, &inc);
    Visit(&op->iter_init, &init);
    Visit(&op->body, &body);
    Var iter = op->iterator;
    *to = For::make(init, cond, inc, body, iter);
  }
  void Visit(const IfThenElse* op, Expr* to) override {
    Expr cond, true_block, false_block;
    Visit(&op->condition, &cond);
    Visit(&op->true_block, &true_block);
    if (op->false_block.valid()) {
      Visit(&op->false_block, &false_block);
      *to = IfThenElse::make(cond, true_block, false_block);
    } else {
      *to = IfThenElse::make(cond, true_block);
    }
  }
  void Visit(const Block* op, Expr* to) override {
    std::vector<ir::Expr> exprs;
    for (auto& expr : op->body) {
      Expr e;
      Visit(&expr, &e);
      exprs.emplace_back(e);
    }

    *to = Block::make(std::move(exprs));
  }
  void Visit(const IntImm* op, Expr* to) override {
    auto b = std::make_shared<IntImm>(*op);
    *to = Expr(b);
  }
  void Visit(const FloatImm* op, Expr* to) override {
    auto b = std::make_shared<FloatImm>(*op);
    *to = Expr(b);
  }
  void Visit(const BoolImm* op, Expr* to) override {
    auto b = std::make_shared<BoolImm>(*op);
    *to = Expr(b);
  }
  void Visit(const Tensor* op, Expr* to) override { *to = Tensor::make(op->dims(), op->ptype(), op->name()); }
  void Visit(const Constant* op, Expr* to) override {
    auto x = std::make_shared<Constant>(*op);
    *to = Expr(x);
  }
  void Visit(const Reference* op, Expr* to) override {
    Expr target;
    std::vector<Expr> iters;
    Visit(&op->target, &target);
    for (auto& iter : op->iterators) {
      Expr i;
      Visit(&iter, &i);
      iters.emplace_back(i);
    }
    to->Reset(Reference::make(target, std::move(iters)));
  }
  void Visit(const Call* op, Expr* to) override {
    std::vector<Expr> iters;
    for (auto& iter : op->arguments) {
      Expr i;
      Visit(&iter, &i);
      iters.emplace_back(i);
    }
    *to = Reference::make(op->caller, std::move(iters));
  }
  void Visit(const Function* op, Expr* to) override {
    std::vector<Expr> inputs, outputs;
    Expr body;

    std::transform(op->inputs.begin(), op->inputs.end(), std::back_inserter(inputs), [this](const Expr& arg) {
      Expr x;
      Visit(&arg, &x);
      return x;
    });
    std::transform(op->outputs.begin(), op->outputs.end(), std::back_inserter(outputs), [this](const Expr& arg) {
      Expr x;
      Visit(&arg, &x);
      return x;
    });

    Visit(&op->body, &body);

    *to = Function::make(op->name(), inputs, outputs, body);
  }
  void Visit(const Statement* op, Expr* to) override { NOT_IMPLEMENT }
  void Visit(const Allocate* op, Expr* to) override {
    Expr size;
    Visit(&op->size, &size);
    *to = Allocate::make(op->buffer_name, size, op->dtype);
  }
  void Visit(const Mark* op, Expr* to) override { *to = Mark::make(op->content); }
  void Visit(const Identity* op, Expr* to) override {
    Expr expr;
    Visit(&op->expr, &expr);
    *to = Identity::make(expr, op->id);
  }
  void Visit(const BufferOpr* op, Expr* to) override {
    Expr size;
    Visit(&op->size, &size);

    *to = BufferOpr::make(op->target, size, op->operation, op->ptype(), op->name);
  }
  void Visit(const Cast* op, Expr* to) override {
    Expr expr;
    Visit(&op->expr, &expr);

    *to = Cast::make(expr, op->ptype(), op->ctype());
  }
  void Visit(const Array* op, Expr* to) override {
    Expr size;
    Visit(&op->size, &size);

    *to = Array::make(size, op->ptype(), op->name);
  }
  void Visit(const SIMDOpr* op, Expr* to) override {
    Expr a, b;
    Visit(&op->a, &a);
    Visit(&op->b, &b);
    CHECK(a);
    CHECK(b);
    *to = SIMDOpr::make(op->vector_width, op->opr, a, b);
  }
  void Visit(const Module* op, Expr* to) override {
    Expr a, b;
    Visit(&op->global_data_section, &a);
    Visit(&op->function_section, &b);
    *to = Module::make(a, b);
  }
  void Visit(const CallOnce* op, Expr* to) override {
    Expr block;
    Visit(&op->block, &block);
    *to = CallOnce::make(block);
  }
};

struct IREqualTeller : public IRVisitorBase<bool, const ir::Expr*> {
  bool Visit(const Expr* a, const Expr* b) override {
    if (a == b || a->ptr() == b->ptr()) return true;
    if (!(a->valid() && b->valid()) && (a->valid() || b->valid())) return false;
    if (a->type() != b->type()) return false;
    return IRVisitorBase::Visit(a, b);
  }

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
  OP_1PARAM(Not);
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
  OP_2PARAM(SumAssign);
  OP_2PARAM(SubAssign);
  OP_2PARAM(MulAssign);
  OP_2PARAM(DivAssign);

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
    if (a->inputs.size() != b->inputs.size()) return false;
    if (a->outputs.size() != b->outputs.size()) return false;

    for (int i = 0; i < a->inputs.size(); i++) {
      if (!Visit(&a->inputs[i], &b->inputs[i])) return false;
    }
    for (int i = 0; i < a->outputs.size(); i++) {
      if (!Visit(&a->outputs[i], &b->outputs[i])) return false;
    }
    return Visit(&a->body, &b->body);
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

    if (a->body.size() != b->body.size()) return false;
    for (int i = 0; i < a->body.size(); i++) {
      if (!Visit(&a->body[i], &b->body[i])) return false;
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

  bool Visit(const BufferOpr* a, const Expr* expr) {
    auto* b = expr->As<BufferOpr>();
    if (a->getptr() == b->getptr()) return true;
    if (a->name != b->name || a->operation != b->operation) return false;
    return Visit(&a->size, &b->size);
  }

  bool Visit(const Array* a, const Expr* expr) {
    auto* b = expr->As<BufferOpr>();
    if (a->getptr() == b->getptr()) return true;
    if (a->name != b->name) return false;
    return Visit(&a->size, &b->size);
  }

  bool Visit(const Stmt* a, const Expr* expr) override { NOT_IMPLEMENT }
  bool Visit(const Tensor* a, const Expr* expr) override {
    auto* b = expr->As<ir::Tensor>();
    if (a->getptr() == b->getptr()) return true;
    if (a == b) return true;
    return a->name() == b->name();
  }
  bool Visit(const Statement* a, const Expr* expr) override { NOT_IMPLEMENT }
  bool Visit(const Allocate* a, const Expr* expr) override { NOT_IMPLEMENT }

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

  bool Visit(const BoolImm* a, const Expr* expr) override {
    auto* b = expr->As<BoolImm>();
    if (a == b) return true;
    return a->val == b->val;
  }

  bool Visit(const Mark* a, const Expr* expr) override {
    auto* b = expr->As<Mark>();
    if (a == b) return true;
    return a->content == b->content;
  }

  bool Visit(const Identity* a, const Expr* expr) override {
    auto* b = expr->As<Identity>();
    if (a == b) return true;
    return a->id == b->id && Visit(&a->expr, &b->expr);
  }

  bool Visit(const Cast* a, const Expr* expr) override {
    auto* b = expr->As<Cast>();
    if (a == b) return true;
    if (a->ptype() != b->ptype()) return false;
    if (a->ctype() != b->ctype()) return false;
    return Visit(&a->expr, &b->expr);
  }

  bool Visit(const SIMDOpr* a, const Expr* expr) override {
    auto* b = expr->As<SIMDOpr>();
    if (a == b) return true;
    if (a->vector_width != b->vector_width || a->opr != b->opr) return false;
    return Visit(&a->a, &b->a) && Visit(&a->b, &b->b);
  }

  bool Visit(const Module* a, const Expr* expr) override {
    auto* b = expr->As<Module>();
    if (a == b) return true;

    int both_valid = a->global_data_section.valid() + b->global_data_section.valid();
    if (both_valid == 1) return false;

    both_valid = a->function_section.valid() + b->function_section.valid();
    if (both_valid == 1) return false;

    if (a->global_data_section.valid() && !Visit(&a->global_data_section, &b->global_data_section)) return false;
    if (a->function_section.valid() && !Visit(&a->function_section, &b->function_section)) return false;

    return true;
  }

  bool Visit(const CallOnce* a, const Expr* expr) override {
    auto* b = expr->As<CallOnce>();
    if (a == b) return true;
    if (a->getptr() == b->getptr()) return true;
    return Visit(&a->block, &b->block);
  }
};

bool IREquals(const Expr& a, const Expr& b) {
  if (a.ptr() == b.ptr()) return true;
  IREqualTeller teller;
  return teller.Visit(&a, &b);
}

ir::Expr IRDeepCopy(const Expr& a) {
  IRCopy copy;
  Expr res;
  copy.Visit(&a, &res);
  return res;
}

namespace {

struct IRReplaceMutator : public ir::IRMutator {
  std::string from_repr;
  ir::Expr to;
  ir::Expr from;

  IRReplaceMutator(ir::Expr from, ir::Expr to) : from(from), to(to), from_repr(ir::Dump(from)) {}

  void Visit(const ir::Expr* expr, ir::Expr* op) override {
    if (ir::Dump(*op) == from_repr) {
      op->Reset(to);
    } else {
      IRMutator::Visit(expr, op);
    }
  }
};

}  // namespace

ir::Expr IRReplace(ir::Expr* source, Expr from, ir::Expr to) {
  IRReplaceMutator mutator(from, to);
  mutator.Visit(source, source);
  return from;
}

std::ostream& operator<<(std::ostream& os, const ir::Expr& x) {
  os << Dump(x);
  return os;
}

std::ostream& operator<<(std::ostream& os, ir::NodeTy type) {
  os << ir::GetNodeTyRepr(type);
  return os;
}

namespace {

/**
 * Simplify an expression.
 *
 * NOTE: Just support +-* /.
 * NOTE: don't support (x + i - i) = (x).
 * NOTE: Neet to relay on GCC to further simplify.
 */
struct IRSimplifyMutator : public ir::IRMutator {
  void Visit(const ir::Expr* expr, ir::Expr* op) override { IRMutator::Visit(expr, op); }

  void Visit(const ir::Add* op, ir::Expr* expr) override {
    auto* node = expr->As<Add>();
    Visit(&node->a, &node->a);
    Visit(&node->b, &node->b);
    if (ConstantCal(node->a, node->b, expr->type(), expr)) return;

    if (IsZeroImm(op->a)) {
      expr->Reset(op->b);
    } else if (IsZeroImm(op->b)) {
      expr->Reset(op->a);
    }
  }
  void Visit(const ir::Sub* op, ir::Expr* expr) override {
    auto* node = expr->As<Sub>();
    Visit(&node->a, &node->a);
    Visit(&node->b, &node->b);
    if (ConstantCal(node->a, node->b, expr->type(), expr)) return;

    if (IsZeroImm(op->a)) {
      if (IsZeroImm(op->b)) {
        expr->Reset(op->b);
      } else {
        expr->Reset(Minus::make(op->b));
      }
    } else if (IsZeroImm(op->b)) {
      expr->Reset(op->a);
    }
  }
  void Visit(const ir::Mul* op, ir::Expr* expr) override {
    auto* node = expr->As<Mul>();
    Visit(&node->a, &node->a);
    Visit(&node->b, &node->b);
    if (ConstantCal(node->a, node->b, expr->type(), expr)) return;

    if (IsZeroImm(op->a)) {  // 0 * x = 0; x * 0 = 0;
      expr->Reset(op->a);
    } else if (IsZeroImm(op->b)) {
      expr->Reset(op->b);
    } else if (IsOneImm(op->a)) {  // 1 * x = x; x * 1 = x
      expr->Reset(op->b);
    } else if (IsOneImm(op->b)) {
      expr->Reset(op->a);
    }
  }
  void Visit(const ir::Div* op, ir::Expr* expr) override {
    auto* node = expr->As<Div>();
    Visit(&node->a, &node->a);
    Visit(&node->b, &node->b);
    if (ConstantCal(node->a, node->b, expr->type(), expr)) return;

    if (IsZeroImm(op->a)) {  // 0 / x = 0; x / 0 and x != 0 is forbidden
      expr->Reset(op->a);
    } else if (IsZeroImm(op->b)) {
      LOG(FATAL) << "zero division detected: " << *expr;
    } else if (IsOneImm(op->b)) {  // x / 1 = x
      expr->Reset(op->a);
    }
  }
  void Visit(const ir::Minus* op, ir::Expr* expr) override {
    auto* node = expr->As<Minus>();
    Visit(&node->a, &node->a);

    if (op->a.is_int_imm()) {
      expr->Reset(Expr(-op->a.As<IntImm>()->val()));
    } else if (op->a.is_float_imm()) {
      expr->Reset(Expr(-static_cast<float>(op->a.As<FloatImm>()->val())));
    }
  }

 protected:
  bool IsZeroImm(const ir::Expr& a) {
    if (a.is_int_imm() && a.As<IntImm>()->val() == 0) return true;
    if (a.is_float_imm() && a.As<FloatImm>()->val() == 0.f) return true;
    return false;
  }

  bool IsOneImm(const ir::Expr& a) {
    if (a.is_int_imm() && a.As<IntImm>()->val() == 1) return true;
    if (a.is_float_imm() && a.As<FloatImm>()->val() == 1.f) return true;
    return false;
  }

  template <typename T>
  T CalConstant(T a, T b, ir::NodeTy ty) {
    switch (ty) {
      case NodeTy::Add:
        return a + b;
      case NodeTy::Sub:
        return a - b;
      case NodeTy::Mul:
        return a * b;
      case NodeTy::Div:
        return a / b;
      default:
        LOG(FATAL) << "Not supported NodeTy " << ty;
    }
  }

  bool ConstantCal(const ir::Expr& a, const ir::Expr& b, ir::NodeTy type, Expr* expr) {
    if (a.is_int_imm() && b.is_int_imm()) {
      expr->Reset(Expr(CalConstant<int>(a.As<IntImm>()->val(), b.As<IntImm>()->val(), expr->type())));
      return true;
    } else if (a.is_float_imm() && b.is_float_imm()) {
      expr->Reset(Expr(CalConstant<float>(a.As<FloatImm>()->val(), b.As<FloatImm>()->val(), expr->type())));
      return true;
    }

    // TODO(Superjomn) Consider the type mismatch scenerios.
    return false;
  }
};

struct IRCountVisitor : public ir::IRVisitor {
  IRCountVisitor(const ir::Expr& target) : target(target) {}

  int operator()(ir::Expr context) {
    Visit(&context);
    return count;
  }

 private:
  int count = 0;
  ir::Expr target;

  void Visit(const Expr* op) override {
    if (ir::Dump(*op) == ir::Dump(target)) {
      ++count;
    } else {
      IRVisitor::Visit(op);
    }
  }
};

}  // namespace

void IRSimplify(ir::Expr* source) {
  IRSimplifyMutator mutator;
  mutator.Visit(source, source);
}

int IRCount(const Expr& context, const Expr& target) {
  IRCountVisitor visitor(target);
  return visitor(context);
}

bool IsConstantFor(const ir::Expr& expr, int* num_elements, int* init_value) {
  LOG_INDENT(6);
  CHECK(expr.is_for_());
  CINN_DEBUG(2) << "checking for\n" << expr;
  auto& for_ = *expr.As<ir::For>();
  // check for's init, cond and extent
  auto* init_val = for_.iter_init.As<ir::IntImm>();
  auto* cond_le = for_.iter_cond.As<ir::LE>();
  auto* inc_val = for_.iter_inc.As<ir::IntImm>();

  *init_value = init_val->val();
  if (!cond_le) {
    CINN_DEBUG(3) << "cond_le fail, cond:" << for_.iter_cond;
    return false;
  }

  if (!cond_le->b.is_int_imm()) {
    CINN_DEBUG(3) << "cond_le value is not IntImm, " << cond_le->b;
    return false;
  }
  if (!inc_val || inc_val->val() != 1) {
    CINN_DEBUG(3) << "for iter_inc != 1, " << for_.iter_inc;
    return false;
  }

  int cond_extent = cond_le->b.As<ir::IntImm>()->val();
  *num_elements = cond_extent - init_val->val() + 1;

  // check the block content
  auto* for_block = for_.body.As<ir::Block>();
  // All the expressions in the block should be vectorizable.
  // NOTE here is just a naive implementation.
  // TODO(Superjomn) enhance here.
  for (auto& expr : for_block->body) {
    if (expr.is_block() || expr.is_for_() || expr.is_if_then_else()) {
      CINN_DEBUG(3) << "found no simple expression:\n" << expr;
      return false;
    }
  }

  CINN_DEBUG(2) << "get constant for";

  return true;
}

void IRCleanRedundantCasts(ir::Expr* expr) {
  struct IRCleanRedundantCastsMutator : public ir::IRMutator {
    void Visit(const ir::Expr* expr, ir::Expr* op) override { IRMutator::Visit(expr, op); }

    void Visit(const Cast* op, Expr* expr) override {
      if (op->ctype() == op->expr.ctype() && op->ptype() == op->expr.ptype()) {
        LOG(INFO) << "xxxxxxxx clean " << *expr;
        expr->Reset(op->expr);
        IRMutator::Visit(expr, expr);
      } else {
        auto* node = expr->As<ir::Cast>();
        Visit(&node->expr, &node->expr);
      }
    }
  };

  IRCleanRedundantCastsMutator mutator;
  mutator.Visit(expr, expr);
}
bool IsBasicExpr(ir::Expr expr) {
  if (!CollectExprNode<ir::Block>(expr).empty()) return false;
  if (!CollectExprNode<ir::SIMDOpr>(expr).empty()) return false;
  if (!CollectExprNode<ir::Assign>(expr).empty()) return false;
  return true;
}

bool BasicExprIdentityVarScale(const Expr& expr, const Expr& var_expr) {
  CHECK(IsBasicExpr(expr));
  CHECK(var_expr.is_var());

  ExprToGinacConveter converter1;
  auto expr_ex = converter1(expr);
  auto var_ex = converter1(var_expr);
  // Check diff(expr, var) == 1
  return expr_ex.diff(converter1.CreateGinacSymbol(var_expr), 1) == GiNaC::ex(1);
}

std::string ExprToGinacConveter::Repr(const Expr& expr) {
  CHECK(expr.is_reference() || expr.is_var());
  std::string repr = GetStreamStr(expr);
  Replace(&repr, "[", "lsq_");
  Replace(&repr, "]", "_rsq");
  Replace(&repr, "(", "lb_");
  Replace(&repr, ")", "_rb");
  Replace(&repr, "+", "_add_");
  Replace(&repr, "-", "_sub_");
  Replace(&repr, "*", "_mul_");
  Replace(&repr, "/", "_div_");
  return repr;
}

void ExprToGinacConveter::RecordExpr(const ir::Expr& expr) {
  CHECK(expr.is_reference() || expr.is_var());
  repr_to_expr_[GetStreamStr(expr)].Reset(expr);
}

GiNaC::ex ExprToGinacConveter::BuildHelper(ir::Expr expr) {
  switch (expr.type()) {
    case ir::NodeTy::Reference:
    case ir::NodeTy::Var: {
      RecordExpr(expr);

      std::string repr = Repr(expr);
      return CreateGinacSymbol(repr);
    }
    case ir::NodeTy::IntImm: {
      auto* x = expr.As<ir::IntImm>();
      return x->val();
    }
    case ir::NodeTy::FloatImm: {
      auto* x = expr.As<ir::FloatImm>();
      return x->val();
    }
    case ir::NodeTy::Add: {
      auto* node = expr.As<ir::Add>();
      auto a = BuildHelper(node->a);
      auto b = BuildHelper(node->b);
      return a + b;
    }
    case ir::NodeTy::Sub: {
      auto* node = expr.As<ir::Sub>();
      return BuildHelper(node->a) - BuildHelper(node->b);
    }
    case ir::NodeTy::Mul: {
      auto* node = expr.As<ir::Mul>();
      return BuildHelper(node->a) * BuildHelper(node->b);
    }
    case ir::NodeTy::Div: {
      auto* node = expr.As<ir::Div>();
      return BuildHelper(node->a) / BuildHelper(node->b);
    }
  }
}

GiNaC::ex ExprToGinacConveter::operator()(Expr expr) {
  CHECK(CollectExprNode<ir::Block>(expr).empty()) << "basic math expression should not contain block";
  CHECK(CollectExprNode<ir::SIMDOpr>(expr).empty());
  return BuildHelper(expr);
}

GiNaC::symbol ExprToGinacConveter::CreateGinacSymbol(const std::string& repr) {
  auto it = repr_to_ginac_.find(repr);
  if (it != repr_to_ginac_.end()) return it->second;

  GiNaC::symbol x(repr);
  repr_to_ginac_[repr] = x;
  return x;
}

GiNaC::symbol ExprToGinacConveter::CreateGinacSymbol(const ir::Expr& var) {
  CHECK(var.is_var());
  return CreateGinacSymbol(Repr(var));
}

bool ReferenceIsAddress(const Expr& expr) {
  auto* identity = expr.As<ir::Identity>();
  if (!identity) return false;
  return identity->marked_as_address();
}

void ExpandAssignOpr(ir::Expr* expr) {
  struct Mutator : ir::IRMutator {
    void Visit(const ir::SumAssign* expr, ir::Expr* op) override {
      op->Reset(ir::Assign::make(expr->a, ir::Add::make(expr->a, expr->b)));
    }
    void Visit(const ir::SubAssign* expr, ir::Expr* op) override {
      op->Reset(ir::Assign::make(expr->a, ir::Sub::make(expr->a, expr->b)));
    }
    void Visit(const ir::MulAssign* expr, ir::Expr* op) override {
      op->Reset(ir::Assign::make(expr->a, ir::Mul::make(expr->a, expr->b)));
    }
    void Visit(const ir::DivAssign* expr, ir::Expr* op) override {
      op->Reset(ir::Assign::make(expr->a, ir::Div::make(expr->a, expr->b)));
    }

    void Visit(const ir::Expr* expr, ir::Expr* op) override { IRMutator::Visit(expr, op); }
  };

  Mutator mutator;
  mutator.Visit(expr, expr);
}

}  // namespace ir
}  // namespace cinn
