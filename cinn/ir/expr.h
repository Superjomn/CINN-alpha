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

  std::shared_ptr<const IRNode> getptr() const { return shared_from_this(); }

  /// Visitor pattern to traverse the IR.
  virtual void Accept(IRVisitor* x) const = 0;

 protected:
  NodeTy type_{NodeTy::Var};
};

/// A handle to store any expression.
class IRHandle {
 protected:
  std::shared_ptr<IRNode> ptr_{};

 public:
  IRHandle() = default;
  IRHandle(IRHandle& other) : ptr_(other.ptr_) {}
  explicit IRHandle(IRNode* x) { ptr_.reset(x); }
  explicit IRHandle(const std::shared_ptr<IRNode>& x) { ptr_ = x; }

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

  void Accept(IRVisitor* visitor) const override { const_self()->Accept(visitor); }

  T* self() { return static_cast<T*>(this); }
  const T* const_self() const { return static_cast<const T*>(this); }
};

template <typename T>
class ExprNode : public ExprNodeBase<T> {
 public:
  ExprNode() = default;
  explicit ExprNode(NodeTy type) : ExprNodeBase<T>(type) {}

  void Accept(IRVisitor* visitor) const override { visitor->Visit(const_self()); }

  T* self() { return static_cast<T*>(this); }
  const T* const_self() const { return static_cast<const T*>(this); }
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

  void Accept(IRVisitor* x) const override { x->Visit(this); }

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
        break;
      default:
        LOG(FATAL) << "unsupported bits " << type.bits() << " for Float";
    }
    return x;
  }

  void Accept(IRVisitor* x) const override { x->Visit(this); }

  float val() const { return val_; }

  static const NodeTy node_type = NodeTy::Float;
};

class Expr : public IRHandle {
 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }

  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  virtual void Accept(IRVisitor* visitor) const { ptr_->Accept(visitor); }

  void operator=(const Expr& other) { ptr_ = other.ptr_; }

  // Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }
};

class Stmt : public IRHandle {
 public:
  Stmt() = default;

  virtual void Accept(IRVisitor* visitor) const {}
};

}  // namespace ir
}  // namespace cinn
