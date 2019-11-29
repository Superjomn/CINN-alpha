#include "cinn/core/optimize/vectorize_utils.h"
#include "cinn/ir/ir_helper.h"

namespace cinn {
namespace optimize {

void Vectorize::operator()(int vector_width, ir::Expr *for_expr) {
  LOG_INDENT(0);
  auto *for_ = for_expr->As<ir::For>();
  CHECK(for_);

  ir::Expr &block_expr = for_expr->As<ir::For>()->body;
  CINN_DEBUG(2) << "vectorize operation:\n" << *for_expr;
  VectorizeOperations(for_expr, vector_width);
  CINN_DEBUG(2) << "result:\n" << *for_expr;

  ir::IRCleanRedundantCasts(for_expr);

  ReplaceForIterToZero(for_expr);

  CINN_DEBUG(2) << "remove for";
  RemoveForloop(for_expr);
  CINN_DEBUG(2) << "result:\n" << *for_expr;
}

void Vectorize::RemoveForloop(ir::Expr *for_expr) {
  CHECK(for_expr->is_for_());
  ir::Expr &body = for_expr->As<ir::For>()->body;
  for_expr->Reset(body);
  CHECK(for_expr->is_block());
}

void CastSimdBasicExprOprArgument(ir::Expr *expr) {
  CHECK(expr->is_impl_normal());
  auto *op = expr->As<ir::SIMDOpr>();
  CHECK(op);

  // make an expression to a SIMD data.
  auto cast_argumet_for_simd_opr = [&](ir::Expr *a, composite_t simd_type) {
    CHECK(IsSimdType(simd_type));
    if (a->is_primitive()) {
      auto cast = ir::Cast::make(*a, a->ptype(), simd_type);
      a->Reset(cast);
    } else if (a->is_simd()) {
      return;
    } else {
      NOT_IMPLEMENT
    }
  };

  switch (op->opr) {
    case ir::SIMDOpr::Opr::kAdd:
    case ir::SIMDOpr::Opr::kSub:
    case ir::SIMDOpr::Opr::kMul:
    case ir::SIMDOpr::Opr::kDiv:
      cast_argumet_for_simd_opr(&op->a, op->ctype());
      cast_argumet_for_simd_opr(&op->b, op->ctype());
      break;
    default:
      NOT_IMPLEMENT
  }
}

namespace {

struct VectorizeOperationsMutator : public ir::IRMutator {
  int vector_width;
  ir::Expr iterator;

  explicit VectorizeOperationsMutator(int vector_width) : vector_width(vector_width) {}
  void operator()(ir::Expr *expr) {
    auto *for_ = expr->As<ir::For>();
    CHECK(for_);
    iterator = for_->iterator;
    Visit(expr, expr);
  }

 private:
  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

  void Visit(const ir::Add *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Add>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }
  void Visit(const ir::Sub *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Sub>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }
  void Visit(const ir::Mul *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Mul>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }
  void Visit(const ir::Div *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Div>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }
  void Visit(const ir::Max *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Max>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }
  void Visit(const ir::Min *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Min>();
    ir::IRMutator::Visit(&node->a, &node->a);
    ir::IRMutator::Visit(&node->b, &node->b);
    DoVectorize(expr);
  }

  void Visit(const ir::Assign *op, ir::Expr *expr) override { VisitAssign(op, expr); }
  void Visit(const ir::SumAssign *op, ir::Expr *expr) override { VisitAssign(op, expr); }
  void Visit(const ir::MulAssign *op, ir::Expr *expr) override { VisitAssign(op, expr); }
  void Visit(const ir::SubAssign *op, ir::Expr *expr) override { VisitAssign(op, expr); }
  void Visit(const ir::DivAssign *op, ir::Expr *expr) override { VisitAssign(op, expr); }

  void Visit(const ir::Reference *op, ir::Expr *expr) override {}
  void Visit(const ir::SIMDOpr *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::SIMDOpr>();
    Visit(&node->a, &node->a);
    Visit(&node->b, &node->b);
  }

  template <typename AssignT>
  void VisitAssign(const AssignT *op, ir::Expr *expr) {
    if (IsReferenceExprSIMDLoadable(op->a, iterator)) {
      auto *store = expr->As<AssignT>();
      Visit(&store->a, &store->a);
      Visit(&store->b, &store->b);

      Expr a = ir::Identity::make(op->a, expr_ids::reference_address);
      Expr simd_store = ir::SIMDOpr::make_store(vector_width, a, store->b);
      expr->Reset(simd_store);
    } else {
      ir::IRMutator::Visit(op, expr);
    }
  }

  void DoVectorize(Expr *expr) {
    LOG_INDENT(6);
    CINN_DEBUG(2) << "*********** Vectorize " << *expr;
    CHECK_GT(vector_width, 1);
    LOG(INFO) << "to vectorize expr: " << *expr;

    auto cast_argument_to_simd = [&](ir::Expr a, composite_t simd_type, ir::Expr iterator) -> ir::Expr {
      if (!a.is_simd()) {
        if (BasicExprVarsCanPassToSIMD(a, iterator)) {
          if (a.is_reference()) {
            a.Reset(ir::Identity::make(a, expr_ids::reference_address));
          } else if (a.is_var() || a.is_float_imm() || a.is_int_imm()) {  // scalar
          } else {
            NOT_IMPLEMENT
          }
        }
        return ir::Cast::make(a, a.ptype(), simd_type);
      }
      return a;
    };

    switch (expr->type()) {
#define __(op__)                                                                   \
  case ir::NodeTy::op__: {                                                         \
    auto *op = expr->As<ir::op__>();                                               \
    auto a = cast_argument_to_simd(op->a, ToSimdType(vector_width), iterator);     \
    auto b = cast_argument_to_simd(op->b, ToSimdType(vector_width), iterator);     \
    expr->Reset(ir::SIMDOpr::make(vector_width, ir::SIMDOpr::Opr::k##op__, a, b)); \
  } break;
      __(Add)
      __(Sub)
      __(Mul)
      __(Div)
      __(Max)
      __(Min)
#undef __
      default:
        LOG(ERROR) << "unsupported " << expr->type();
    }
  }
};

}  // namespace

void Vectorize::VectorizeOperations(ir::Expr *block_expr, int vector_width) {
  VectorizeOperationsMutator mutator(vector_width);
  mutator(block_expr);
}

void Vectorize::ReplaceForIterToZero(ir::Expr *for_expr) {
  auto *for_ = for_expr->As<ir::For>();
  ir::IRReplace(for_expr, for_->iterator, Expr(0));
}

//! NOTE Only support basic expressions.
bool BasicExprContainsOnlySIMDReleatedOpr(const Expr &expr) {
  LOG_INDENT(0);
  CHECK(ir::CollectExprNode<ir::Block>(expr).empty());
  // Check if the operations(not in a reference) can be represented by SIMD.
  struct AllOprCanSIMD : public ir::IRVisitor {
    std::set<ir::NodeTy> supported_ops{ir::NodeTy::Add,
                                       ir::NodeTy::Sub,
                                       ir::NodeTy::Mul,
                                       ir::NodeTy::Div,
                                       ir::NodeTy::Max,
                                       ir::NodeTy::Min,
                                       ir::NodeTy::Assign,
                                       ir::NodeTy::SumAssign,
                                       ir::NodeTy::SubAssign,
                                       ir::NodeTy::MulAssign,
                                       ir::NodeTy::DivAssign};

    void Visit(const Expr *op) override {
      switch (op->type()) {
        case ir::NodeTy::Reference:
        case ir::NodeTy::Var:
        case ir::NodeTy::IntImm:
        case ir::NodeTy::FloatImm:
          return;
      }

      if (!supported_ops.count(op->type())) {
        CINN_DEBUG(2) << "detect SIMD not supported operation: " << *op;
        result = false;
      } else {
        IRVisitor::Visit(op);
      }
    }

    void Visit(const ir::Reference *op) override {}

    bool result{true};
    bool in_reference{false};
  };

  AllOprCanSIMD visitor;
  visitor.Visit(&expr);
  return visitor.result;
}

bool BasicExprVarsCanPassToSIMD(const Expr &basic_expr, const Expr &iterator) {
  CHECK(ir::CollectExprNode<ir::Block>(basic_expr).empty()) << "not a basic expression";

  auto refs = ir::CollectExprNode<ir::Reference>(basic_expr);
  for (auto &ref_expr : refs) {
    // check the positions but the last one
    auto *ref = ref_expr.As<ir::Reference>();
    for (int i = 0; i < ref->iterators.size() - 1; i++) {
      if (ir::IREquals(ref->iterators[i], iterator)) {
        return false;
      }
    }
  }

  // all the other cases works
  return true;
}

bool Vectorizable(const Expr &expr, const std::set<int> &vectorize_widths, int *vector_width) {
  LOG_INDENT(0);

  int init_value;
  if (!ir::IsConstantFor(expr, vector_width, &init_value)) return false;
  if (init_value != 0) {
    CINN_DEBUG(3) << "fail, init_val != 0, " << init_value;
    return false;
  }

  if (!ir::CollectExprNode<ir::SIMDOpr>(expr).empty()) {
    CINN_DEBUG(3) << "fail, already have SIMD opr";
    return false;
  }

  // check all the expressions in the for-block can represented by SIMD operations.
  auto *for_ = expr.As<ir::For>();
  for (auto &basic_expr : for_->body.As<ir::Block>()->body) {
    if (!BasicExprContainsOnlySIMDReleatedOpr(basic_expr)) {
      CINN_DEBUG(3) << "fail, detect operation that can't represent by SIMD, " << basic_expr;
      return false;
    }

    // check the argument used in the basic expressions.
    Expr iterator_expr = Expr(for_->iterator);
    CHECK(iterator_expr.is_var());
    if (!BasicExprVarsCanPassToSIMD(basic_expr, iterator_expr)) {
      CINN_DEBUG(3) << "fail, detect variable in the operation can't supported by SIMD, " << basic_expr;
      return false;
    }
  }

  return true;
}

template <typename IrAssignT>
void CastAssignLeftSimdArgument(ir::Expr *expr) {
  auto *op = expr->As<IrAssignT>();
  CHECK(op);
  CHECK(op->a.is_simd());
  CHECK(!op->b.is_simd());
  CHECK(op->b.is_primitive());
  // argument a is a SIMD
  // argument b is a scalar
  auto cast = ir::Cast::make(op->b, op->a.ptype(), op->a.ctype());
  op->b.Reset(cast);
}

template <>
void CastAssignLeftSimdArgument<ir::Assign>(ir::Expr *expr);
template <>
void CastAssignLeftSimdArgument<ir::SumAssign>(ir::Expr *expr);
template <>
void CastAssignLeftSimdArgument<ir::SubAssign>(ir::Expr *expr);
template <>
void CastAssignLeftSimdArgument<ir::MulAssign>(ir::Expr *expr);
template <>
void CastAssignLeftSimdArgument<ir::DivAssign>(ir::Expr *expr);

void CastAssignRightSimdArgument(ir::Expr *expr) {
  auto *op = expr->As<ir::Assign>();
  CHECK(op);
  CHECK(op->a.is_primitive());
  CHECK(op->b.is_simd());
  CHECK(op->a.ptype() == op->b.ptype());

  auto cast = ir::Cast::make(op->b, op->b.ptype(), composite_t::primitive);
  op->b.Reset(cast);
}

bool IsReferenceExprSIMDLoadable(const Expr &expr, ir::Expr iterator) {
  auto *reference = expr.As<ir::Reference>();
  CHECK(reference);
  CHECK(!reference->iterators.empty());
  // check that this argument is a valid SIMD Opr argument.
  for (int i = 0; i < reference->iterators.size() - 1; i++) {
    // A[.. i ..] can not cast to SIMD in i-th forloop.
    if (ir::BasicExprIdentityVarScale(reference->iterators[i], iterator)) return false;
  }
  // Check the last indice is something like 1 + i, with coefficient of i is 1
  return ir::BasicExprIdentityVarScale(reference->iterators.back(), iterator);
}

bool IsSimdData(ir::Expr expr) {
  if (expr.is_simd()) return true;

  std::vector<std::string> ids;
  auto *identity = expr.As<ir::Identity>();
  if (!identity) return false;
  identity->GetTrimedExpr(&ids);
  return Found(ids, std::string(expr_ids::reference_address));
}

}  // namespace optimize
}  // namespace cinn
