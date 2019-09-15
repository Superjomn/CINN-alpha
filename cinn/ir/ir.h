#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cinn/ir/expr.h"
#include "cinn/type.h"
#include "cinn/utils/any.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/macros.h"
#include "cinn/utils/name_generator.h"

namespace cinn {
namespace ir {

class Constant : public ExprNode<Constant> {
  std::string name_;
  primitive_t primitive_type_{primitive_t::unk};
  union {
    int8_t int8_val_;
    int32_t int32_val_;
    int64_t int64_val_;
    float fp32_val_;
    double fp64_val_;
  };

 public:
  Constant() = default;
  Constant(const std::string& name, primitive_t type) : name_(name), primitive_type_(type) {}
  Constant(const std::string& name, int32_t val) : name_(name), primitive_type_(primitive_t::int32), int32_val_(val) {}
  Constant(const Constant& other);

  template <typename T>
  Constant(const std::string& name, T val);
  template <typename T>
  Constant(T val);

  primitive_t primitive_type() const { return primitive_type_; }

  bool valid() const { return primitive_type_ != primitive_t::unk; }

  bool is_integer() const {
    return primitive_type() == primitive_t::int32 || primitive_type() == primitive_t::int8 ||
           primitive_type() == primitive_t::int16 || primitive_type() == primitive_t::int64;
  }

  bool operator==(const Constant& other) const {
    return name_ == other.name_ && primitive_type() == other.primitive_type();
  }

  std::string __str__() const;

  template <typename T>
  T As() const;

  static const NodeTy node_type = NodeTy::Parameter;

 private:
  // Generate a random default name.
  std::string DefaultUniqueName() { return "p" + std::to_string(counter++); }

  static unsigned int counter;
};

/*
 * Interval for Var and Parameter.
 */
class Interval {
 public:
  Interval() = default;
  Interval(Constant lower_bound, Constant upper_bound) : lower_bound_(lower_bound), upper_bound_(upper_bound) {}

  const Constant& lower_bound() const { return lower_bound_; }
  const Constant& upper_bound() const { return upper_bound_; }

  bool operator==(const Interval& other) const {
    return lower_bound() == other.lower_bound() && upper_bound() == other.upper_bound();
  }

  std::string __str__() const {
    std::stringstream ss;
    ss << "Interval";
    if (lower_bound().valid()) ss << "(" << lower_bound().__str__();
    if (upper_bound().valid()) ss << ", " << upper_bound().__str__() << ")";
    return ss.str();
  }

 private:
  Constant lower_bound_;
  Constant upper_bound_;
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
  primitive_t data_type_;
  Interval interval_;
  std::string name_;
  primitive_t dtype_{primitive_t::unk};

  static size_t counter_;
  static std::set<std::string> name_set_;  // All registerred var's name here.

 public:
  Var() { name_ = NameGenerator::Global().NewVarName(); }

  //! Create a varaible with UNK data type. The dtype should be inferenced latter with the context.
  Var(const std::string& name) : name_(name) { CheckNameValid(name); }

  Var(const std::string& name, primitive_t dtype) : name_(name), dtype_(dtype) { CheckNameValid(name); }

  // make a variable with name and interval set.
  Var(const std::string& name, primitive_t type, const Interval& interval)
      : name_(name), data_type_(type), interval_(interval) {
    CheckNameValid(name_);
  }

  Var(const std::string& name, int32_t lower_bound, int32_t upper_bound);

  Var(const std::string& name, primitive_t type, Constant lower_bound, Constant upper_bound)
      : name_(name), data_type_(type), interval_(lower_bound, upper_bound) {
    CheckNameValid(name);
  }

  Var(const Var& other) : name_(other.name()), dtype_(other.dtype()), interval_(other.interval()) {}

  operator Expr();

  primitive_t dtype() const { return dtype_; }

  bool operator==(const Var& other) const { return name() == other.name() && interval() == other.interval(); }

  void Accept(IRVisitor* x) const override {}

  const std::string& name() const { return name_; }

  static const NodeTy node_type = NodeTy::Var;

  const Interval& interval() const { return interval_; }

  std::string __repr__() const;

 private:
  static bool CheckNameValid(const std::string& name);
};

/**
 * Expr is a basic concept of CINN, everything in the IR is an Expr.
 */
class Expr : public IRHandle {
  std::vector<Var> iterators_;

 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }
  Expr(const std::string& name, primitive_t dtype = primitive_t::float32) { *this = Var(name, dtype); }

  // reference
  Expr(const std::vector<Var>& its) : iterators_(its) {}

  Expr Assign(Expr other);

  //! Create an int32 Expr.
  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  //! Create an int64 Expr.
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  //! Create an float Expr.
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  /**
   * Element assignment.
   *
   * Usage:
   *
   *     Expr A, B, C;
   *     Var i("i", {0, M}), j("j", {0, N});
   *     A(i,j) = B(i,j) + C(i,j) * 2;
   *
   * Will get ISL map:
   *
   * - domain: `[M, N] -> { 0 <= i < M and j : 0 <= j < N }`
   * - statement: `A[i,j] = B[i,j] + C[i,j] * 2`
   *
   * @param iters list of iterators.
   * @return Expr.
   */
  Expr operator[](std::vector<Var> iters);
  Expr operator[](Var i);
  Expr operator[](Expr i);
  Expr operator[](std::vector<Expr> iters);
  // Expr operator[](Var i, Var j) { return (*this)({i, j}); }
  // Expr operator[](Var i, Var j, Var k) { return (*this)({i, j, k}); }
  // Expr operator()(Var i, Var j, Var k, Var l) { return (*this)({i, j, k, l}); }

  virtual void Accept(IRVisitor* visitor) const { ptr_->Accept(visitor); }

  /**
   * Assign an Expr from other or create an ir::Assign node representing the assignment of an ir::Reference node.
   *
   * It has two cases:
   *
   * 1. Expr assign, e.g. `Expr C = A`.
   * 2. Reference assignment, e.g. `C(i,j) = A(i,j) + 1`.
   */
  Expr operator=(const Expr& other);

  //! Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }

  //! Tell whether this expression is a operator.
  bool is_op() const;

#define IS_TYPE(m__, ty__) \
  bool is_##m__() const { return type() == ir::NodeTy::ty__; }
  IS_TYPE(var, Var)
  IS_TYPE(function, Function)
  IS_TYPE(allocate, Allocate)
  IS_TYPE(assign, Assign)
#undef IS_TYPE
};

/**
 * Reference is the reference of an element of an Expr.
 *
 * For example:
 *
 *     Expr A;
 *     Var i("i", {0, N});
 *     Var j("i", {0, N});
 *     // Get a reference of A[i,j]
 *     Expr C = A(i, j); // Now C is a Reference.
 */
struct Reference : public ExprNode<Reference> {
  using interval_tuple_t = std::tuple<std::string, Interval>;
  //! the reference target.
  Expr target;
  //! the iterators of the element.
  std::vector<Expr> iterators;

  Reference() = default;

  void Accept(IRVisitor* x) const override {
    x->Visit(&target);
    for (auto& iter : iterators) {
      x->Visit(&iter);
    }
  }

  //! Extract intervals from the reference of all the dimention iterators.
  std::vector<interval_tuple_t> ExtractIntervals();

  //! Create a Reference Expr.
  static Expr make(Expr expr, const std::vector<Expr>& iterators);

  static const NodeTy node_type = NodeTy::Reference;
};

/**
 * Tensor is a placeholder for the inputs of the whole program.
 */
class Tensor : public ExprNode<Tensor> {
  std::string name_;
  primitive_t type_;
  std::vector<Constant> dims_;

 public:
  Tensor(const std::string& name, primitive_t type, const std::vector<Constant>& dims)
      : name_(name), type_(type), dims_(dims) {}

  /*
Expr operator()(Var i) { return Reference::make(Expr(), {i}); }
Expr operator()(Var i, Var j) { return Reference::make(Expr(), {i, j}); }
Expr operator()(Var i, Var j, Var z) { return Reference::make(Expr(), {i, j, z}); }
Expr operator()(Var i, Var j, Var z, Var k) { return Reference::make(Expr(), {i, j, z, k}); }
   */

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Tensor;
};

//-------------------- Arithmetical expressions -------------------------
struct Add : public ExprNode<Add> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Add;
};

struct Sub : public ExprNode<Sub> {
 public:
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Sub;
};

struct Mul : public ExprNode<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Mul;
};

struct Div : public ExprNode<Div> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Div;
};

struct Mod : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Mod;
};

struct Min : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Min;
};

struct Max : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Max;
};

struct Minus : public ExprNode<Minus> {
  Expr a;

  static Expr make(Expr a);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Minus;
};

//-------------------- Logical expressions -------------------------
struct EQ : public ExprNode<EQ> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::EQ;
};

/// Not equal.
struct NE : public ExprNode<NE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::NE;
};

struct LE : public ExprNode<LE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::LE;
};

struct LT : public ExprNode<LT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::LT;
};

struct GT : public ExprNode<GT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::GT;
};

struct GE : public ExprNode<GE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::GE;
};

struct And : public ExprNode<And> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::And;
};

struct Or : public ExprNode<Or> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Or;
};

// Block of code.
struct Block : public ExprNode<Block> {
  // statements(Expr) in the block.
  std::vector<Expr> exprs;

  static Expr make(std::vector<Expr>&& list);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Block;
};

struct IfThenElse : public ExprNode<IfThenElse> {
  Expr condition;
  Expr true_block;
  Expr false_block;

  // Only has the true condition.
  static Expr make(Expr condition, Expr true_block);
  // Has both the true and false condition.
  static Expr make(Expr condition, Expr true_block, Expr false_block);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::IfThenElse;
};

struct For : public ExprNode<For> {
  Expr iter_init, iter_cond, iter_inc;
  Expr body;
  Var iterator;

  static Expr make(Expr iter_init, Expr iter_cond, Expr iter_inc, Expr body, Var iterator);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::For;
};

struct Call : public ExprNode<Call> {
  std::vector<Expr> arguments;
  std::string caller;

  static Expr make(const std::string& caller, std::vector<Expr> arguments);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Call;
};

struct Assign : public ExprNode<Assign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Assign;
};

/**
 * Statement is the statement of CINN IR, it is a special kind of Expr, which not return a value.
 * It is used to make `Stage` more natual to embed in IR.
 */
class Statement : public ExprNode<Statement> {
 public:
  Expr expr;

  Statement() = default;

  void Accept(IRVisitor* x) const override { x->Visit(this); }

  static Expr make(Expr expr) {
    auto node = std::make_shared<Statement>();
    node->expr = expr;
    return Expr(node);
  }

  static const NodeTy node_type = NodeTy::Statement;
};

class Allocate : public ExprNode<Allocate> {
 public:
  std::string buffer_name;
  Expr size;
  primitive_t dtype{primitive_t::unk};

  Allocate() = default;

  static Expr make(const std::string& buffer_name, Expr size, primitive_t dtype);

  static const NodeTy node_type = NodeTy::Allocate;
};

/**
 * @brief Param is the parameter in polyhedral model, the constant value across the program.
 *
 * @attention the Param's data type should be int.
 */
class Param : public ir::ExprNode<Param> {
  struct Data {
    /// name of the parameter, should be unique accross the Function.
    std::string name;
    /// the condition that can represent the condition in ISL.
    /// e.g. a Param with name of "N" and cond of "N>0" will get "[N]->{ : N>0 }" in ISL syntax.
    std::string cond;
  };

  std::shared_ptr<Data> data_;

 public:
  Param() : data_(std::make_shared<Data>()) {}
  Param(const std::string& name, const std::string& cond = "");

  isl::set GetContext();

  const std::string& name() const;
  const std::string& cond() const;

  virtual void Accept(ir::IRVisitor* visitor) const {}

  static const ir::NodeTy node_type = ir::NodeTy::Param;
};

}  // namespace ir
}  // namespace cinn
