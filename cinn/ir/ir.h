#pragma once
#include <lcms2_plugin.h>
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
  std::string name_;
  Any val_;
  ScalarT data_type_;
  Interval interval_;

 public:
  // make a variable with name and interval set.
  Var(const std::string& name, ScalarT type, const Interval& interval)
      : name_(name), data_type_(type), interval_(interval) {}

  // make string constants
  static Var make(const std::string& x);

  // make int32 constraint
  static Var make(int32_t x);

  // make int64 constraint
  static Var make(int64_t x);

  // make float constraint
  static Var make(float x);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Var;
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
