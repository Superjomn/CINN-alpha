#pragma once

#include <string>
#include <vector>
#include "cinn/ref_pointer.h"
#include "cinn/type.h"

namespace cinn {
namespace ir {

/// All the node types supported by cinn.
enum class NodeType {
  Int32,
  UInt32,
  Float32,
  String,

  // Mathematical ones
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Min,
  Max,

  // Conditional ones
  EQ,
  NE,
  LT,
  LE,
  GT,
  GE,
  And,
  Or,
  Not,
};

/// The base class for all the IR nodes.
class Node {
 public:
  Node(NodeType type) : type_(type) {}

 private:
  NodeType type_;
};

/// A handle to store any expression.
class IRHandle {
 public:
};

class ExprNode {};

class StmtNode {};

/// Integer constants
class IntNode : ExprNode {
  int64_t val_;

 public:
  static IntNode* make(NodeType type, int64_t val) {
    auto* x = new IntNode;
    x->val_ = val;
  }
};

class HandleBase : public Referenced, RefPointer<HandleBase> {
 public:
  template <typename T>
  T& As();
};

class Expr : public IRHandle {
 public:
};

}  // namespace ir
}  // namespace cinn
