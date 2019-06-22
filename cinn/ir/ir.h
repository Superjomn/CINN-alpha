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

class Var : public ExprNode<Var> {
  std::string name_;
  Any val_;
  ScalarT data_type_;

 public:
  // constants
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
  static Var make(int64_t x);
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

    auto* x = new Mul;
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
