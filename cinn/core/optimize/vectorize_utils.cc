#include "cinn/core/optimize/vectorize_utils.h"

namespace cinn {
namespace optimize {

/**
 * Collect the arguments of SIMDOpr in a single block(not nested).
 *
 * NOTE The references that can be used as arguments of SIMD should following the rules:
 * 1. the innermost forloop will be transformed to the SIMD operations, name the corresponding iterator i
 *   1. a reference with i as the last indice should be casted to a SIMD argument,
 *   2. a reference whose indices don't contains i can be casted to a SIMD scalar (e.g. use _m128_set1_ps),
 *   3. a reference with i in the other position, the whole block should not be vectorized.
 *   4. a reference used as the left oprand in an assign expression, if is not a SIMD argument following the previous
 * rules, the right oprand should be casted to scalar(by accumulate).
 *      1. e.g. C[j] += A[i] * B[i]
 *
 *
 *
 */
struct SimdReferenceCollector : public ir::IRVisitor {
  std::map<std::string, Expr> arguments;

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

  void Visit(const ir::Assign *op) override {
    if (op->b.is_simd_opr()) {
      if (op->a.is_reference()) {
        CollectReference(op->a);
      }
    }

    ir::IRVisitor::Visit(op);
  }

  void Visit(const ir::SIMDOpr *op) override {
    if (op->a.is_reference()) {
      CollectReference(op->a);
    }
    if (op->b.is_reference()) {
      CollectReference(op->b);
    }

    ir::IRVisitor::Visit(op);
  }

  void Visit(const ir::Block *op) override {
    for (auto &expr : op->body) {
      // This should works with the "nested_block_clean_pass".
      Visit(&expr);  // avoid nested
    }
  }

  void CollectReference(const ir::Expr &a) {
    auto key = ir::Dump(a);
    if (arguments.count(key)) {
      arguments[key].set_ptr(a.ptr());
    } else {
      arguments[ir::Dump(a)] = a;
    }
  }
};

/**
 * Replace the SIMD op's arguments with the casted variables.
 */
struct SimdArgumentReplacer : public ir::IRMutator {
  const std::map<std::string, ir::Expr /* var */> &ref_repr_to_var;

  explicit SimdArgumentReplacer(const std::map<std::string, ir::Expr> &x) : ref_repr_to_var(x) {}

  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

 private:
  void Visit(const ir::Assign *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::Assign>();
    if (op->b.is_simd_opr()) {
      ReplaceSimdArgument(&node->a);
    }

    IRMutator::Visit(&node->b, &node->b);
  }

  void Visit(const ir::SIMDOpr *op, ir::Expr *expr) override {
    auto *node = expr->As<ir::SIMDOpr>();
    ReplaceSimdArgument(&node->a);
    ReplaceSimdArgument(&node->b);

    IRMutator::Visit(op, expr);
  }

  void Visit(const ir::Block *op, Expr *expr) override {
    auto *node = expr->As<ir::Block>();
    for (auto &e : node->body) {
      if (!e.is_block()) {
        ir::IRMutator::Visit(&e, &e);
      }
    }
  }

 protected:
  void ReplaceSimdArgument(ir::Expr *expr) {
    for (auto &item : ref_repr_to_var) {
      if (ir::Dump(*expr) == item.first) {
        expr->set_ptr(item.second.ptr());
        break;
      }
    }
  }
};

/**
 * Insert the cast expressions for SIMD arguments.
 */
struct SimdArgumentCaster : public ir::IRMutator {
  int vector_width{-1};

  const std::map<std::string, ir::Expr> *arguments{};
  std::map<std::string, ir::Expr> ref_to_var;

  explicit SimdArgumentCaster(int vector_width) : vector_width{vector_width} {}

  void operator()(ir::Expr *block_expr, const std::map<std::string, ir::Expr> *simd_ref_args) {
    CHECK(!simd_ref_args->empty());
    arguments = simd_ref_args;
    Visit(block_expr, block_expr);
  }

 private:
  void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }
  void Visit(const ir::Block *op, Expr *expr) override {
    auto *block = expr->As<ir::Block>();

    CHECK(arguments) << "argments should be set first";
    PreappendCastToBlock(expr->As<ir::Block>(), *arguments);

    IRMutator::Visit(op, expr);
  }

 protected:
  void PreappendCastToBlock(ir::Block *block, const std::map<std::string, ir::Expr> &simd_args) {
    composite_t simd_type = composite_t::primitive;
    if (vector_width > 0) {
      if (vector_width == 4) {
        simd_type = composite_t::simd128;
      } else if (vector_width == 8) {
        simd_type = composite_t::simd256;
      } else {
        LOG(FATAL) << "not support vector_width " << vector_width;
      }
    }

    for (const auto &item : simd_args) {
      LOG(INFO) << "collected " << item.first << " -> " << item.second;
      auto cast = ir::Cast::make(item.second, item.second.ptype());
      cast.set_ctype(simd_type);
      ir::Var var(GlobalContext().name_generator().NewVarName());
      var.set_is_reference();
      Expr var_expr(var);

      ref_to_var[item.first] = var_expr;

      auto let = ir::Let::make(var_expr, cast);
      let.set_ctype(simd_type);

      // Preappend cast expressions to block.
      block->body.insert(std::begin(block->body), let);
    }
  }
};

void Vectorize::operator()(int vector_width, ir::Expr *for_expr) {
  LOG_INDENT(0);
  CHECK(for_expr->is_for_());

  ir::Expr &block_expr = for_expr->As<ir::For>()->body;
  CINN_DEBUG(2) << "vectorize operation";
  VectorizeOprerations(&block_expr, vector_width);
  CINN_DEBUG(2) << "result:\n" << *for_expr;

  auto simd_refs = CollectSimdReferences(block_expr);
  CHECK(!simd_refs.empty());
  CINN_DEBUG(2) << "collected simd references: " << simd_refs.size();

  auto ref_to_vars = InsertSimdCastExprToBlock(&block_expr, vector_width, simd_refs);
  CHECK(!ref_to_vars.empty());
  CINN_DEBUG(2) << "collected reference to variables: " << ref_to_vars.size();

  ReplaceSimdArgument(ref_to_vars, &block_expr);
  CINN_DEBUG(2) << "result:\n" << *for_expr;

  ReplaceForIterToZero(for_expr);

  CINN_DEBUG(2) << "remove for";
  RemoveForloop(for_expr);
  CINN_DEBUG(2) << "result:\n" << *for_expr;
}

std::map<std::string, ir::Expr> Vectorize::CollectSimdReferences(const ir::Expr &block_expr) {
  CHECK(block_expr.is_block());
  SimdReferenceCollector collector;
  collector.Visit(&block_expr);
  return collector.arguments;
}

std::map<std::string, ir::Expr> Vectorize::InsertSimdCastExprToBlock(ir::Expr *block_expr,
                                                                     int vector_width,
                                                                     const std::map<std::string, Expr> &simd_ref_args) {
  CHECK(block_expr->is_block());
  SimdArgumentCaster mutator(vector_width);
  mutator(block_expr, &simd_ref_args);
  return mutator.ref_to_var;
}

void Vectorize::ReplaceSimdArgument(const std::map<std::string, Expr> &ref_to_vars, ir::Expr *block_expr) {
  CHECK(block_expr->is_block());
  SimdArgumentReplacer mutator(ref_to_vars);
  mutator.Visit(block_expr, block_expr);
}

void Vectorize::RemoveForloop(ir::Expr *for_expr) {
  CHECK(for_expr->is_for_());
  ir::Expr &body = for_expr->As<ir::For>()->body;
  for_expr->Reset(body);
  CHECK(for_expr->is_block());
}

void Vectorize::VectorizeOprerations(ir::Expr *block_expr, int vector_width) {
  struct Mutator : public ir::IRMutator {
    bool inside_reference_ = false;
    int vector_width;

    explicit Mutator(int vector_width) : vector_width(vector_width) {}
    void operator()(ir::Expr *expr) { Visit(expr, expr); }

   private:
    void Visit(const ir::Expr *expr, ir::Expr *op) override { IRMutator::Visit(expr, op); }

    void Visit(const ir::Add *op, ir::Expr *expr) override {
      if (!inside_reference_) {
        Vectorize(expr);
        ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
      } else {
        ir::IRMutator::Visit(op, expr);
      }
    }
    void Visit(const ir::Sub *op, ir::Expr *expr) override {
      if (!inside_reference_) {
        Vectorize(expr);
        ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
      } else {
        ir::IRMutator::Visit(op, expr);
      }
    }
    void Visit(const ir::Mul *op, ir::Expr *expr) override {
      if (!inside_reference_) {
        Vectorize(expr);
        ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
      } else {
        ir::IRMutator::Visit(op, expr);
      }
    }
    void Visit(const ir::Div *op, ir::Expr *expr) override {
      if (!inside_reference_) {
        Vectorize(expr);
        ir::IRMutator::Visit(expr->As<ir::SIMDOpr>(), expr);
      } else {
        ir::IRMutator::Visit(op, expr);
      }
    }
    void Visit(const ir::Reference *op, ir::Expr *expr) override {
      inside_reference_ = true;
      ir::IRMutator::Visit(op, expr);
      inside_reference_ = false;
    }

    void Vectorize(Expr *expr) {
      LOG_INDENT(6);
      CINN_DEBUG(2) << "*********** Vectorize " << *expr;
      CHECK_GT(vector_width, 1);
      switch (expr->type()) {
#define __(op__)                                                                      \
  case ir::NodeTy::op__: {                                                            \
    auto *op = expr->As<ir::op__>();                                                  \
    *expr = ir::SIMDOpr::make(vector_width, ir::SIMDOpr::Opr::k##op__, op->a, op->b); \
  } break;
        __(Add)
        __(Sub)
        __(Mul)
        __(Div)
#undef __
        default:
          LOG(ERROR) << "unsupported " << expr->type();
      }

      LOG(INFO) << "after vectorize " << *expr;
    }
  };

  Mutator mutator(vector_width);
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
    std::set<ir::NodeTy> supported_ops{
        ir::NodeTy::Add, ir::NodeTy::Sub, ir::NodeTy::Mul, ir::NodeTy::Div, ir::NodeTy::Assign};

    void Visit(const Expr *op) override {
      if (op->type() == ir::NodeTy::Reference) return;
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
}  // namespace optimize
}  // namespace cinn
