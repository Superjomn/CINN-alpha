#pragma once

/*
 * We borrows many concepts from Halide/IR.
 */

#include <glog/logging.h>
#include <memory>
#include <string>
#include <vector>
#include "cinn/ir/node_base.h"
#include "cinn/type.h"
#include "cinn/utils/macros.h"

namespace cinn {
namespace ir {

/// All the node types supported by cinn.
enum class NodeTy {
  __START__ = -1,
#define DECL_ENUM_ITEM(x__) x__,
  NODETY_FOR_EACH(DECL_ENUM_ITEM)
#undef DECL_ENUM_ITEM
      __NUM__,
};

static const std::string& GetNodeTyRepr(NodeTy ty) {
#define NODETY_REPR(x__) #x__,
  static std::vector<std::string> NodeTyReprs{{NODETY_FOR_EACH(NODETY_REPR)}};
#undef NODETY_REPR
  return NodeTyReprs[static_cast<int>(ty)];
}

static size_t GetNodeTyNum() { return static_cast<size_t>(NodeTy::__NUM__); }

struct IRVisitor;

/// The base class for all the IR nodes.
class IRNode : public std::enable_shared_from_this<IRNode> {
 public:
  explicit IRNode(NodeTy type) : type_(type) {}

  std::shared_ptr<const IRNode> getptr() const { return shared_from_this(); }

  NodeTy type() const { return type_; }

  //! Get the primitive type of this expr. Each expr has a type, even the operators, so that it can convert to a LLVM IR
  //! which
  //! has type.
  primitive_t ptype() const { return ptype_; }
  composite_t ctype() const { return ctype_; }

  void set_ptype(primitive_t type) { ptype_ = type; }
  void set_ctype(composite_t type) { ctype_ = type; }

  bool is_unk() const { return ptype() == primitive_t::unk; }
  bool is_boolean() const { return ptype() == primitive_t::boolean; }
  bool is_m128() const { return ctype_ == composite_t::simd128; }

  virtual ~IRNode() = default;

 protected:
  NodeTy type_{NodeTy::Var};
  primitive_t ptype_{primitive_t::unk};
  composite_t ctype_{composite_t::primitive};
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

  NodeTy type() const {
    CHECK(ptr_);
    return ptr_->type();
  }

  template <typename T>
  const T* As() const {
    // TODO(Superjomn) check the type
    if (ptr_) return static_cast<const T*>(ptr_.get());
    return nullptr;
  }

  template <typename T>
  T* As() {
    // TODO(Superjomn) check the type
    if (ptr_) return static_cast<T*>(ptr_.get());
    return nullptr;
  }

  void Reset() { ptr_.reset(); }

  const std::shared_ptr<IRNode>& ptr() const { return ptr_; }
  void set_ptr(const std::shared_ptr<IRNode>& x) { ptr_ = x; }
};

/// Base class of all the Exprs. An Expr is an expression that can return a value, such as `a+1`.
// TODO(Superjomn) this class seems useless, drop it.
template <typename T>
class ExprNodeBase : public IRNode {
 public:
  ExprNodeBase() : IRNode(T::node_type) {}
  explicit ExprNodeBase(NodeTy type) : IRNode(type) {}

  T* self() { return static_cast<T*>(this); }
  const T* const_self() const { return static_cast<const T*>(this); }
};

template <typename T>
class ExprNode : public ExprNodeBase<T> {
 public:
  ExprNode() = default;
  explicit ExprNode(NodeTy type) : ExprNodeBase<T>(type) {}

  T* self() { return static_cast<T*>(this); }
  const T* const_self() const { return static_cast<const T*>(this); }
};

/// Base class of all the Stmts. A Stmt is a statement that do not represent a value (e.g. assert(a>10)).
template <typename T>
class StmtNodeBase : public IRNode {
 public:
  StmtNodeBase() : IRNode(T::_node_type) {}
  explicit StmtNodeBase(NodeTy type) : IRNode(type) {}
};

/// Integer constants
class IntImm : public ExprNodeBase<IntImm> {
  int64_t val_;
  Type type_;

 public:
  static std::shared_ptr<IntImm> make(Type type, int64_t val) {
    auto node = std::make_shared<IntImm>();
    node->type_ = type;
    node->val_ = val;
    node->set_ptype(primitive_t::int64);
    return node;
  }

  static std::shared_ptr<IntImm> make(Type type, int32_t val) {
    auto node = std::make_shared<IntImm>();
    node->type_ = type;
    node->val_ = val;
    node->set_ptype(primitive_t::int32);
    return node;
  }

  int64_t val() const { return val_; }
  const Type& type() const { return type_; }

  static const NodeTy node_type = NodeTy::IntImm;
};

/// Float constants
class FloatImm : public ExprNodeBase<FloatImm> {
  double val_;
  Type type_;

 public:
  static std::shared_ptr<FloatImm> make(Type type, float val) {
    auto node = std::make_shared<FloatImm>();
    node->set_ptype(primitive_t::float32);
    node->type_ = type;

    switch (type.bits()) {
      case 16:
      case 32:
      case 64:
        node->val_ = val;
        break;
      default:
        LOG(FATAL) << "unsupported bits " << type.bits() << " for Float";
    }
    return node;
  }

  float val() const { return val_; }

  static const NodeTy node_type = NodeTy::FloatImm;
};

class Stmt : public IRHandle {
 public:
  Stmt() = default;
};

}  // namespace ir

static std::ostream& operator<<(std::ostream& os, ir::NodeTy type) {
  os << ir::GetNodeTyRepr(type);
  return os;
}

}  // namespace cinn
