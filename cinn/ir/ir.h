#pragma once
#include <string>
#include <vector>

#include "cinn/ir/expr.h"
#include "cinn/type.h"

namespace cinn {
namespace ir {

//-------------------- Arithmetical expressions -------------------------
struct Add : public ExprNode<Add> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeType _node_type = NodeType::Add;
};

struct Sub : public ExprNode<Sub> {
 public:
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeType _node_type = NodeType::Sub;
};

struct Mul : public ExprNode<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeType _node_type = NodeType::Mul;
};

//-------------------- Logical expressions -------------------------
struct EQ : public ExprNode<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeType _node_type = NodeType::EQ;
};

struct NE : public ExprNode<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeType _node_type = NodeType::NE;
};

}  // namespace ir
}  // namespace cinn
