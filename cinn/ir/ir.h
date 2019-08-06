#pragma once
#include <string>
#include <vector>
#include "cinn/ir/expr.h"
#include "cinn/type.h"
#include "cinn/utils/any.h"

namespace cinn {
namespace ir {

enum class ScalarT {
  int32,
  int64,
  float32,
  string,
};

class Parameter : public ExprNode<Parameter> {
  std::string name_;
  ScalarT type_;
  int32_t int32_val_;

 public:
  Parameter() = default;
  Parameter(const std::string& name, ScalarT type) : name_(name), type_(type) {}
  Parameter(const std::string& name, int32_t val) : name_(name), type_(ScalarT::int32), int32_val_(val) {}

  static const NodeTy node_type = NodeTy::Parameter;
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
  ScalarT data_type_;
  Interval interval_;
  std::string name_;

  primitive_t primitive_type_;

  static size_t counter_;
  static std::set<std::string> name_set_;  // All registerred var's name here.

 public:
  Var() { SetDefaultName(); }

  // make a variable with name and interval set.
  Var(const std::string& name, ScalarT type, const Interval& interval)
      : name_(name), data_type_(type), interval_(interval) {
    inc_counter();
    check_set_name(name_);
  }

  Var(const std::string& name, ScalarT type, Parameter lower_bound, Parameter upper_bound)
      : name_(name), data_type_(type), interval_(lower_bound, upper_bound) {}

  primitive_t primitive_type() const { return primitive_type_; }

  void Accept(IRVisitor* x) const override {}

  static bool check_set_name(const std::string& name) {
    if (name_set_.count(name)) {
      name_set_.insert(name);
      return true;
    }
    return false;
  }

  void SetDefaultName() {
    name_ = "var" + std::to_string(inc_counter());
    CHECK(check_set_name(name_));
  }

  static const NodeTy node_type = NodeTy::Var;

  static size_t inc_counter() { return counter_++; }
};

class Expr : public IRHandle {
  std::vector<Var> iterators_;

 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }

  // reference
  Expr(const std::vector<Var>& its) : iterators_(its) {}

  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  virtual void Accept(IRVisitor* visitor) const { ptr_->Accept(visitor); }

  void operator=(const Expr& other) { ptr_ = other.ptr_; }

  // Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }
};

class Reference : public ExprNodeBase<Reference> {
  std::vector<Var> iters_;

 public:
  Reference() = default;

  void Accept(IRVisitor* x) const override { x->Visit(this); }
  static const NodeTy node_type = NodeTy::Reference;

  static std::shared_ptr<IRNode> make(const std::vector<Var>& iters) {
    auto x = std::make_shared<Reference>();
    x->iters_ = iters;
    return x;
  }
};

/*
 * Tensor is a placeholder for the inputs of the whole program.
 */
class Tensor : public ExprNode<Tensor> {
  std::string name_;
  ScalarT type_;
  std::vector<Parameter> dims_;

 public:
  Tensor(const std::string& name, ScalarT type, const std::vector<Parameter>& dims)
      : name_(name), type_(type), dims_(dims) {}

  Expr operator()(Var i) { return Reference::make({i}); }
  Expr operator()(Var i, Var j) { return Reference::make({i, j}); }
  Expr operator()(Var i, Var j, Var z) { return Reference::make({i, j, z}); }
  Expr operator()(Var i, Var j, Var z, Var k) { return Reference::make({i, j, z, k}); }

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Tensor;
};

//-------------------- Arithmetical expressions -------------------------
struct Add : public ExprNode<Add> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Add;
};

struct Sub : public ExprNode<Sub> {
 public:
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {
    a.Accept(x);
    b.Accept(x);
  }

  static const NodeTy node_type = NodeTy::Sub;
};

struct Mul : public ExprNode<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b) {
    CHECK(a.valid()) << "Mul a not defined";
    CHECK(b.valid()) << "Mul b not defined";

    auto x = std::make_shared<Mul>();
    x->a = std::move(a);
    x->b = std::move(b);
    return Expr(x);
  }

  static const NodeTy node_type = NodeTy::Mul;
};

struct Div : public ExprNode<Div> {
  Expr a, b;

  static Expr make(Expr a, Expr b) {
    CHECK(a.valid()) << "Div a not defined";
    CHECK(b.valid()) << "Div b not defined";

    auto x = std::make_shared<Div>();
    x->a = std::move(a);
    x->b = std::move(b);
    return Expr(x);
  }

  static const NodeTy node_type = NodeTy::Div;
};

struct Mod : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Mod;
};

//-------------------- Logical expressions -------------------------
struct EQ : public ExprNode<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::EQ;
};

/// Not equal.
struct NE : public ExprNode<NE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::NE;
};

/*
class Computation : public ExprNode<Computation> {
  std::vector<Var> inters_;
  Expr expr_;

 public:
  Computation(Var i) : inters_({i}) {}
  Computation(Var i, Var j) : inters_({i, j}) {}
  Computation(Var i, Var j, Var k) : inters_({i, j, k}) {}

  void operator=(Expr expr) { expr_ = expr; }

  static const NodeTy node_type = NodeTy::Computation;
};
 */

}  // namespace ir
}  // namespace cinn
