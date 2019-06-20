#pragma once

/*
 * We borrows many concepts from Halide/IR.
 */

#include <glog/logging.h>
#include <memory>
#include <string>
#include <vector>
#include "cinn/ir/ir_visitor.h"
#include "cinn/ir/node_base.h"
#include "cinn/type.h"

namespace cinn {
namespace ir {

/// All the node types supported by cinn.
enum class NodeTy {
  Int,
  UInt,
  Float,
  String,

  // Mathematical ones Add,
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

  Var,
};

/// The base class for all the IR nodes.
class IRNode : public std::enable_shared_from_this<IRNode> {
 public:
  explicit IRNode(NodeTy type) : type_(type) {}

  std::shared_ptr<IRNode> getptr() { return shared_from_this(); }

  /// Visitor pattern to traverse the IR.
  virtual void Accept(IRVisitor* x) const = 0;

 protected:
  NodeTy type_;
};

/// A handle to store any expression.
class IRHandle {
 protected:
  std::shared_ptr<const IRNode> ptr_;

 public:
  IRHandle() = default;
  IRHandle(const IRHandle& other) : ptr_(other.ptr_) {}
  explicit IRHandle(const IRNode* x) : ptr_(x) {}

  template <typename T>
  const T* As() const {
    // TODO(Superjomn) check the type
    if (ptr_) return static_cast<const T*>(ptr_.get());
    return nullptr;
  }
};

/// Base class of all the Exprs. An Expr is an expression that can return a value, such as `a+1`.
template <typename T>
class ExprNodeBase : public IRNode {
 public:
  ExprNodeBase() : IRNode(T::node_type) {}
  explicit ExprNodeBase(NodeTy type) : IRNode(type) {}

  void Accept(IRVisitor* visitor) const override {}
};

/// Base class of all the Stmts. A Stmt is a statement that do not represent a value (e.g. assert(a>10)).
template <typename T>
class StmtNodeBase : public IRNode {
 public:
  StmtNodeBase() : IRNode(T::_node_type) {}
  explicit StmtNodeBase(NodeTy type) : IRNode(type) {}

  void Accept(IRVisitor* visitor) const override {}
};

/// Integer constants
class IntImm : public ExprNodeBase<IntImm> {
  int64_t val_;
  Type type_;

 public:
  static std::shared_ptr<IntImm> make(Type type, int64_t val) {
    auto x = std::make_shared<IntImm>();
    x->type_ = type;
    x->val_ = val;
    return x;
  }

  void Accept(IRVisitor* x) const override {}

  int64_t val() const { return val_; }
  const Type& type() const { return type_; }

  static const NodeTy node_type = NodeTy::Int;
};

/// Float constants
class FloatImm : public ExprNodeBase<FloatImm> {
  double val_;
  Type type_;

 public:
  static std::shared_ptr<FloatImm> make(Type type, float val) {
    auto x = std::make_shared<FloatImm>();
    x->type_ = type;

    switch (type.bits()) {
      case 16:
      case 32:
      case 64:
        x->val_ = val;
      default:
        LOG(FATAL) << "unsupported bits " << type.bits() << "for Float";
    }
    return x;
  }

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Float;
};

class Expr : public IRHandle {
 public:
  Expr() {}
  Expr(const IRNode* n) : IRHandle(n) {}  // NOLINT

  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  virtual void Accept(IRVisitor* visitor) const {}

  // Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }
};

}  // namespace ir
}  // namespace cinn
