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

class Var : public Expr {
  std::string name_;
  Any val_;
  ScalarT data_type_;

 public:
  // constants
  static Var make(const std::string& x) {
    Var v;
    v.data_type_ = ScalarT::string;
    v.val_.set(x);
  }
  static Var make(int32_t x) {
    Var v;
    v.data_type_ = ScalarT::int32;
    v.val_.set(x);
  }
  static Var make(int64_t x);
  static Var make(float x);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Var;
};

//-------------------- Arithmetical expressions -------------------------
struct Add : public ExprNodeBase<Add> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Add;
};

struct Sub : public ExprNodeBase<Sub> {
 public:
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Sub;
};

struct Mul : public ExprNodeBase<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Mul;
};

//-------------------- Logical expressions -------------------------
struct EQ : public ExprNodeBase<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::EQ;
};

struct NE : public ExprNodeBase<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::NE;
};

}  // namespace ir
}  // namespace cinn
