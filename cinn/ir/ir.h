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

/*
 * Interval for Var and Parameter.
 */
class Interval {
 public:
  Interval() = default;
  Interval(Expr lower_bound, Expr upper_bound) : lower_bound_(lower_bound), upper_bound_(upper_bound) {}

  const Expr& lower_bound() const { return lower_bound_; }
  const Expr& upper_bound() const { return upper_bound_; }

  std::string __str__() const {}

 private:
  Expr lower_bound_;
  Expr upper_bound_;
};

class Tensor : public ExprNode<Tensor> {
 public:
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

  // lower bound.
  Expr lower_;
  // upper bound.
  Expr upper_;

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

  // make string constants
  static Var make(const std::string& x) {
    Var v;
    v.data_type_ = ScalarT::string;
    v.val_.set(x);
    return v;
  }
  static Var make(int32_t x) {
    Var v;
    v.data_type_ = ScalarT::int32;
    v.val_.set(x);
    return v;
  }

  primitive_t primitive_type() const { return primitive_type_; }

  void Accept(IRVisitor* x) const override;

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

  static Expr make(Expr a, Expr b);

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

}  // namespace ir
}  // namespace cinn
