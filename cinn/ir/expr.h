#pragma once

/*
 * We borrows many concepts from Halide/IR.
 */

#include <string>
#include <vector>
#include "cinn/ir/ir_visitor.h"
#include "cinn/ref_pointer.h"
#include "cinn/type.h"

namespace cinn {
namespace ir {

/// All the node types supported by cinn.
enum class NodeType {
  Int,
  UInt,
  Float,
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

  /// Visitor pattern to traverse the IR.
  void Accept(IRVisitor* x) const = 0;

 private:
  NodeType type_;
};

/// A handle to store any expression.
class IRHandle : public Referenced, RefPointer<ir::Node> {
 public:
  IRHandle() : RefPointer<ir::Node>() {}
  IRHandle(Node* node) : RefPointer<ir::Node>(node) {}

  template <typename T>
  T& As() {
    // TODO(Superjomn) check the type
    if (ptr_) return static_cast<T*>(ptr_);
    return nullptr;
  }
};

template <typename T>
class ExprNode : public Node {
 public:
  ExprNode() : Node(T::_node_type) {}
  ExprNode(NodeType type) : Node(type) {}
};

template <typename T>
class StmtNode : public Node {
 public:
  StmtNode() : Node(T::_node_type) {}
  StmtNode(NodeType type) : Node(type) {}
};

/// Integer constants
class IntImm : public ExprNode<IntImm> {
  int64_t val_;

 public:
  static IntImm* make(NodeType type, int64_t val) {
    auto* x = new IntImm;
    x->val_ = val;
  }

  static const NodeType _node_type = NodeType::Int;
};

/// Float constants
class FloatImm : public ExprNode<FloatImm> {
  double val_;

 public:
  static FloatImm* make(Type type, float val) {
    auto* x = new FloatImm;

    switch (type.bits()) {
      case 16:
      case 32:
      case 64:
        val_ = val;
    }
    x->val_ = val;
  }

  static const NodeType _node_type = NodeType::Float;
};

class Expr : public IRHandle {
 public:
  Expr() : IRHandle() {}
  template <typename T>
  Expr(const ExprNode<T>* n) : IRHandle(n) {}
};

}  // namespace ir
}  // namespace cinn
