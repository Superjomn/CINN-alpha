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

struct Buffer;

namespace ir {

class Constant : public ExprNode<Constant> {
  std::string name_;
  union {
    int8_t int8_val_;
    int32_t int32_val_;
    int64_t int64_val_;
    float fp32_val_;
    double fp64_val_;
  };

 public:
  Constant() = default;
  Constant(const std::string& name, primitive_t type) : name_(name) { set_ptype(type); }
  Constant(const std::string& name, int32_t val) : name_(name), int32_val_(val) { set_ptype(primitive_t::int32); }
  Constant(const Constant& other);

  template <typename T>
  Constant(const std::string& name, T val);
  template <typename T>
  Constant(T val);

  operator Expr();

  bool valid() const { return ptype() != primitive_t::unk; }

  bool is_integer() const {
    return ptype() == primitive_t::int32 || ptype() == primitive_t::int8 || ptype() == primitive_t::int16 ||
           ptype() == primitive_t::int64;
  }

  bool is_float() const { return ptype() == primitive_t::float32 || ptype() == primitive_t::float64; }

  bool operator==(const Constant& other) const { return name_ == other.name_ && ptype() == other.ptype(); }

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
  struct Data {
    Any val_;
    Interval interval_;
    std::string name_;
    std::unique_ptr<isl::set> domain_;
  };

  std::shared_ptr<Data> data_;

  static size_t counter_;
  static std::set<std::string> name_set_;  // All registerred var's name here.

 public:
  Var() {
    InitData();
    // set as iterator by default.
    set_ptype(primitive_t::int32);
    data_->name_ = NameGenerator::Global().NewIteratorName();
  }

  //! Create a varaible with UNK data type. The dtype should be inferenced latter with the context.
  Var(const std::string& name) {
    InitData();
    data_->name_ = name;
    // treat as iterator by default.
    set_ptype(primitive_t::int32);
    CheckNameValid(name);
  }

  Var(const std::string& name, primitive_t dtype) {
    InitData();
    data_->name_ = name;
    CheckNameValid(name);
    set_ptype(dtype);
  }

  // make a variable with name and interval set.
  Var(const std::string& name, primitive_t type, const Interval& interval) {
    InitData();
    data_->name_ = name;
    set_ptype(type);
    data_->interval_ = interval;
    CheckNameValid(data_->name_);
  }

  Var(const std::string& name, int32_t lower_bound, int32_t upper_bound);

  Var(const std::string& name, primitive_t type, Constant lower_bound, Constant upper_bound) {
    InitData();
    data_->name_ = name;
    set_ptype(type);
    data_->interval_ = Interval(lower_bound, upper_bound);
    CheckNameValid(name);
  }

  Var(const Var& other) {
    data_ = other.data_;
    set_ptype(other.ptype());
  }

  operator Expr();

  bool operator==(const Var& other) const {
    return data_ == other.data_ || (name() == other.name() && interval() == other.interval());
  }

  void Accept(IRVisitor* x) const override {}

  const std::string& name() const { return data_->name_; }
  void set_name(const std::string& name) { data_->name_ = name; }

  static const NodeTy node_type = NodeTy::Var;

  const Interval& interval() const { return data_->interval_; }

  bool is_domain_valid() const { return data_->domain_.get() && !data_->domain_->is_null(); }
  void set_domain(const isl::set& domain) {
    if (!data_->domain_.get()) data_->domain_.reset(new isl::set);
    *data_->domain_ = domain;
  }
  const isl::set& domain() const {
    CHECK(is_domain_valid());
    return *data_->domain_;
  }

  std::string __repr__() const;

 private:
  void InitData() { data_ = std::make_shared<Data>(); }
  static bool CheckNameValid(const std::string& name);
};

/**
 * Expr is a basic concept of CINN, everything in the IR is an Expr.
 */
class Expr : public IRHandle {
  std::vector<Var> iterators_;
  Buffer* buffer_{nullptr};

 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }
  Expr(const std::string& name, primitive_t dtype = primitive_t::float32) { *this = Var(name, dtype); }

  Expr Assign(Expr other);

  //! Create an int32 Expr.
  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  //! Create an int64 Expr.
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  //! Create an float Expr.
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }

  //! Construct from dimentions, and get a Tensor.
  explicit Expr(const std::vector<Constant>& dims, primitive_t ptype, const std::string& name = "");

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
  // Expr operator[](std::vector<Var> iters);
  // Expr operator[](Var i);
  Expr operator[](Expr i);
  // Expr operator[](std::vector<Expr> iters);

  Expr operator()(const std::vector<Expr>& iters);

  primitive_t ptype() const { return ptr_->ptype(); }
  void set_ptype(primitive_t type) { ptr_->set_ptype(type); }
  bool is_unk() const { return ptr_->is_unk(); }
  bool is_boolean() const { return ptr_->is_boolean(); }

  void Bind(Buffer& buffer) { buffer_ = &buffer; }
  bool buffer_binded() const { return buffer_; }

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
  IS_TYPE(allocate, Allocate)
  IS_TYPE(reference, Reference)
  IS_TYPE(assign, Assign)
  IS_TYPE(function_call, Call)
  IS_TYPE(tensor, Tensor)
  IS_TYPE(function, Function)
  IS_TYPE(block, Block)
#undef IS_TYPE

  virtual void Accept(IRVisitor* visitor) const { ptr_->Accept(visitor); }

 private:
  // Inference the dimention indice on the id-th dimention.
  void InferenceIteratorDomain(Expr* expr);
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
  //! the dimension of the reference.
  std::vector<Expr> dims;

  isl::set domain;

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

 private:
  void InferenceDimsFromIterators();

  void InferenceIteratorDims();
};

/**
 * Tensor is a placeholder for the inputs of the whole program.
 */
class Tensor : public ExprNode<Tensor> {
  std::string name_;
  std::vector<Constant> dims_;

 public:
  Tensor(const std::string& name, primitive_t type, const std::vector<Constant>& dims) : name_(name), dims_(dims) {
    set_ptype(type);
  }

  const std::string& name() const { return name_; }
  const std::vector<Constant>& dims() const { return dims_; }

  static Expr make(const std::vector<Constant>& dims, primitive_t type, const std::string& name) {
    auto node = std::make_shared<Tensor>(name.empty() ? NameGenerator::Global().NewVarName() : name, type, dims);
    return Expr(node);
  }

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

struct Min : public ExprNode<Min> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  void Accept(IRVisitor* x) const override;

  static const NodeTy node_type = NodeTy::Min;
};

struct Max : public ExprNode<Max> {
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

struct Exp : public ExprNode<Exp> {
  Expr a;

  static Expr make(Expr a) {
    CHECK(!a.is_unk());
    auto node = std::make_shared<Exp>();
    node->a = a;
    node->set_ptype(a.ptype());
    return Expr(node);
  }

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Exp;
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

  bool is_store() const { return a.is_reference(); }

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

  //! get the isl context of parameter constraits.
  isl::set context() const;

  virtual void Accept(ir::IRVisitor* visitor) const {}

  static const ir::NodeTy node_type = ir::NodeTy::Param;
};

// Math functions.

class Tanh : public ir::ExprNode<Tanh> {
 public:
  Expr a;

  static Expr make(const Expr& e) {
    auto node = std::make_shared<Tanh>();
    node->a = e;
    return Expr(node);
  }

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Tanh;
};

class Sigmoid : public ir::ExprNode<Tanh> {
 public:
  Expr a;

  static Expr make(const Expr& e) {
    auto node = std::make_shared<Tanh>();
    node->a = e;
    return Expr(node);
  }

  void Accept(IRVisitor* x) const override {}

  static const NodeTy node_type = NodeTy::Sigmoid;
};

//! Extract the Vars from a expression.
std::vector<Var> ExtractVarsFromExpr(const Expr& expr);

//! Build a domain considering all the dimensions.
// e.g. [ M, N, K ] {:} will get { [i0, i1, i2] : 0 <= i0 <= M and 0 <= i1 <= i1 and 0 <= i2 <= K }
isl::set BuildDomainFromDimensions(const std::vector<Constant>& dims, const std::vector<std::string>& iter_names);

//! Generate iterator's name.
// e.g. get i2 if id=2
inline std::string GenIndexedIteratorName(int id);

isl::set BuildDomainFromExprWithDimension(const std::vector<Expr>& exprs, const std::vector<Constant>& dims);

}  // namespace ir
}  // namespace cinn
