#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cinn/ir/expr.h"
#include "cinn/type.h"
#include "cinn/utils/any.h"

namespace cinn {
namespace ir {

class Parameter : public ExprNode<Parameter> {
  std::string name_;
  primitive_t type_;
  union {
    int8_t int8_val_;
    int32_t int32_val_;
    float fp32_val_;
    double fp64_val_;
  };

 public:
  Parameter() = default;
  Parameter(const std::string& name, primitive_t type) : name_(name), type_(type) {}
  Parameter(const std::string& name, int32_t val) : name_(name), type_(primitive_t::int32), int32_val_(val) {}

  template <typename T>
  Parameter(const std::string& name, T val);
  template <typename T>
  Parameter(T val);

  static const NodeTy node_type = NodeTy::Parameter;

 private:
  // Generate a random default name.
  std::string DefaultUniqueName() { return "p" + std::to_string(counter++); }

  static unsigned int counter;
};

/*
 * Interval for Var and Parameter.
 */
class Interval {
 public:
  Interval() = default;
  Interval(Parameter lower_bound, Parameter upper_bound) : lower_bound_(lower_bound), upper_bound_(upper_bound) {}

  const Parameter& lower_bound() const { return lower_bound_; }
  const Parameter& upper_bound() const { return upper_bound_; }

  std::string __str__() const { return "Interval"; }

 private:
  Parameter lower_bound_;
  Parameter upper_bound_;
};

/*
 * Var is a variable in IR.
 *
 * Usage:
 *
 * Var i, j;
 * Param M("M"), N("N");
 * Interval row(0, M-1);
 * Interval col(0, N-1);
 * Tensor tensor("image"), {M, N});
 *
 * Var tmp;
 * tmp(i,j) = tensor(i,j) / 255;
 *
 * Function func({i,j}, {col, row}, tmp);
 *
 */
class Var : public ExprNode<Var> {
  Any val_;
  primitive_t data_type_;
  Interval interval_;
  std::string name_;

  primitive_t primitive_type_;

  static size_t counter_;
  static std::set<std::string> name_set_;  // All registerred var's name here.

 public:
  Var() { SetDefaultName(); }

  Var(const std::string& name) : name_(name) {}

  // make a variable with name and interval set.
  Var(const std::string& name, primitive_t type, const Interval& interval)
      : name_(name), data_type_(type), interval_(interval) {
    inc_counter();
    check_set_name(name_);
  }

  Var(const std::string& name, primitive_t type, Parameter lower_bound, Parameter upper_bound)
      : name_(name), data_type_(type), interval_(lower_bound, upper_bound) {}

  operator Expr();

  primitive_t primitive_type() const { return primitive_type_; }

  void Accept(IRVisitor* x) const override {}

  const std::string& name() const { return name_; }

  static const NodeTy node_type = NodeTy::Var;

 private:
  static bool check_set_name(const std::string& name) {
    if (!name_set_.count(name)) {
      name_set_.insert(name);
      return true;
    }
    return false;
  }

  void SetDefaultName() {
    name_ = "var" + std::to_string(inc_counter());
    CHECK(check_set_name(name_));
  }

  static size_t inc_counter() { return counter_++; }
};

/**
 * Expr is a basic concept of CINN, everything in the IR is an Expr.
 */
class Expr : public IRHandle {
  std::vector<Var> iterators_;

 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }

  // reference
  Expr(const std::vector<Var>& its) : iterators_(its) {}

  //! Create an int32 Expr.
  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  //! Create an int64 Expr.
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  //! Create an float Expr.
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  /**
   * Element assignment.
   *
   * Usage:
   *
   *     Expr A, B, C;
   *     Var i("i", {0, M}), j("j", {0, N});
   *     A(i,j) = B(i,j) + C(i,j) * 2;
   *
   * Will get ISL map:
   *
   * - domain: `[M, N] -> { 0 <= i < M and j : 0 <= j < N }`
   * - statement: `A[i,j] = B[i,j] + C[i,j] * 2`
   *
   * @param iters list of iterators.
   * @return Expr.
   */
  Expr operator()(std::vector<Expr> iters);

  virtual void Accept(IRVisitor* visitor) const { ptr_->Accept(visitor); }

  /**
   * Assign an Expr from other or create an ir::Assign node representing the assignment of an ir::Reference node.
   *
   * It has two cases:
   *
   * 1. Expr assign, e.g. `Expr C = A`.
   * 2. Reference assignment, e.g. `C(i,j) = A(i,j) + 1`.
   */
  Expr operator=(const Expr& other);

  //! Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }
};

/**
 * Reference is the reference of an element of an Expr.
 *
 * For example:
 *
 *     Expr A;
 *     Var i("i", {0, N});
 *     Var j("i", {0, N});
 *     // Get a reference of A[i,j]
 *     Expr C = A(i, j); // Now C is a Reference.
 */
struct Reference : public ExprNode<Reference> {
  //! the reference target.
  Expr target;
  //! the iterators of the element.
  std::vector<Var> iterators;

  Reference() = default;

  void Accept(IRVisitor* x) const override { x->Visit(this); }
  static const NodeTy node_type = NodeTy::Reference;

  //! Create a Reference Expr.
  Expr make(Expr expr, const std::vector<Var>& iters);
};

/**
 * Tensor is a placeholder for the inputs of the whole program.
 */
class Tensor : public ExprNode<Tensor> {
  std::string name_;
  primitive_t type_;
  std::vector<Parameter> dims_;

 public:
  Tensor(const std::string& name, primitive_t type, const std::vector<Parameter>& dims)
      : name_(name), type_(type), dims_(dims) {}

  /*
Expr operator()(Var i) { return Reference::make(Expr(), {i}); }
Expr operator()(Var i, Var j) { return Reference::make(Expr(), {i, j}); }
Expr operator()(Var i, Var j, Var z) { return Reference::make(Expr(), {i, j, z}); }
Expr operator()(Var i, Var j, Var z, Var k) { return Reference::make(Expr(), {i, j, z, k}); }
   */

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Tensor;
};

//-------------------- Arithmetical expressions -------------------------
struct Add : public ExprNode<Add> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Add;
};

struct Sub : public ExprNode<Sub> {
 public:
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Sub;
};

struct Mul : public ExprNode<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Mul;
};

struct Div : public ExprNode<Div> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Div;
};

struct Mod : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Mod;
};

struct Min : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Min;
};

struct Max : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Max;
};

struct Minus : public ExprNode<Minus> {
  Expr a;

  static Expr make(Expr a);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Minus;
};

//-------------------- Logical expressions -------------------------
struct EQ : public ExprNode<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::EQ;
};

/// Not equal.
struct NE : public ExprNode<NE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::NE;
};

struct LE : public ExprNode<LE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::LE;
};

struct LT : public ExprNode<LT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::LT;
};

struct GT : public ExprNode<GT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::GT;
};

struct GE : public ExprNode<GE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::GE;
};

struct And : public ExprNode<And> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::And;
};

struct Or : public ExprNode<Or> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Or;
};

// Block of code.
struct Block : public ExprNode<Block> {
  std::vector<Expr> list;

  static Expr make(std::vector<Expr>&& list);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Block;
};

struct IfThenElse : public ExprNode<IfThenElse> {
  Expr condition;
  Expr true_block;
  Expr false_block;

  // Only has the true condition.
  static Expr make(Expr condition, Expr true_block);
  // Has both the true and false condition.
  static Expr make(Expr condition, Expr true_block, Expr false_block);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::IfThenElse;
};

struct For : public ExprNode<For> {
  Expr iter_init, iter_cond, iter_inc;
  Expr body;
  Var iterator;

  static Expr make(Expr iter_init, Expr iter_cond, Expr iter_inc, Expr body, Var iterator);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::For;
};

struct Call : public ExprNode<Call> {
  std::vector<Expr> arguments;
  std::string caller;

  static Expr make(const std::string& caller, std::vector<Expr> arguments);

  static const NodeTy node_type = NodeTy::Call;
};

struct Assign : public ExprNode<Assign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Assign;
};

}  // namespace ir
}  // namespace cinn
