#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/cinn_context.h"
#include "cinn/ir/expr.h"
#include "cinn/target.h"
#include "cinn/type.h"
#include "cinn/utils/any.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/macros.h"
#include "cinn/utils/name_generator.h"

namespace cinn {

namespace ir {
struct Expr;
struct Buffer;

/**
 * A constant value of some primitive type.
 */
class Constant : public ExprNode<Constant> {
  std::string name_;
  union {
    int8_t int8_val_;
    int16_t int16_val_;
    int32_t int32_val_;
    int64_t int64_val_;
    float float32_val_;
    double float64_val_;
    bool bool_var_;
  };
  bool value_set_{false};

 public:
  Constant() = default;
  Constant(const std::string& name, primitive_t type) : name_(name) { set_ptype(type); }
  Constant(const std::string& name, int32_t val) : name_(name), int32_val_(val), value_set_(true) {
    set_ptype(primitive_t::int32);
  }
  Constant(const Constant& other);

  template <typename T>
  Constant(const std::string& name, T val) {
    name_ = name;
    SetValue(val);
  }
  template <typename T>
  Constant(T val) {
    SetValue(val);
  }

  template <typename T>
  void SetValue(T v);

  const std::string& name() const { return name_; }

  operator Expr();

  bool valid() const { return ptype() != primitive_t::unk; }

  bool is_integer() const {
    return ptype() == primitive_t::int32 || ptype() == primitive_t::int8 || ptype() == primitive_t::int16 ||
           ptype() == primitive_t::int64;
  }

  bool is_float() const { return ptype() == primitive_t::float32 || ptype() == primitive_t::float64; }

  /**
   * Tell whether two Constant equals.
   * @param other the other Constant instance to compare.
   * @return boolean represents whether two Constants equals.
   */
  bool operator==(const Constant& other) const;

  int64_t int_val() const;
#define __(type__)                         \
  int32_t type__##_val() const {           \
    CHECK(ptype() == primitive_t::type__); \
    CHECK(value_set_);                     \
    return type__##_val_;                  \
  }
  __(int16)
  __(int32)
  __(int64)
  __(float32)
  __(float64)
#undef __

  bool value_set() const { return value_set_; }

  std::string __str__() const;

  template <typename T>
  T As() const;

  static const NodeTy node_type = NodeTy::Constant;

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

  std::string __str__() const;

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
    // Is a reference of an expression.
    bool is_reference{false};
  };

  std::shared_ptr<Data> data_;

  static std::set<std::string> name_set_;  // All registerred var's name here.

 public:
  Var();

  //! Create a varaible with UNK data type. The dtype should be inferenced latter with the context.
  Var(const std::string& name) {
    InitData();
    data_->name_ = name;
    // treat as iterator by default.
    set_ptype(primitive_t::int32);
    CheckNameValid(name);
  }

  Var(const std::string& name, primitive_t dtype);

  // make a variable with name and interval set.
  Var(const std::string& name, primitive_t type, const Interval& interval);

  Var(const std::string& name, int32_t lower_bound, int32_t upper_bound);

  Var(const std::string& name, primitive_t type, Constant lower_bound, Constant upper_bound);

  Var(const Var& other) {
    data_ = other.data_;
    set_ptype(other.ptype());
    set_ctype(other.ctype());
  }

  operator Expr();

  bool operator==(const Var& other) const {
    return data_ == other.data_ || (name() == other.name() && interval() == other.interval());
  }

  void set_is_reference(bool x = true) { data_->is_reference = x; }
  bool is_reference() const { return data_->is_reference; }

  const std::string& name() const { return data_->name_; }
  void set_name(const std::string& name) { data_->name_ = name; }

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

  static const NodeTy node_type = NodeTy::Var;

 private:
  void InitData() { data_ = std::make_shared<Data>(); }
  static bool CheckNameValid(const std::string& name);
};

/**
 * Expr(Expression) is a basic concept of CINN, everything in the IR can be an Expr.
 */
class Expr : public IRHandle {
  std::vector<Var> iterators_;

 public:
  Expr() : IRHandle() {}
  Expr(const std::shared_ptr<IRNode>& x) : IRHandle(x) {}
  Expr(const Expr& n) : IRHandle(n.ptr_) {}
  Expr(Expr&& other) { ptr_ = std::move(other.ptr_); }
  Expr(const std::string& name, primitive_t dtype = primitive_t::float32) { *this = Expr(Var(name, dtype)); }

  Expr Assign(Expr other) const;
  Expr SumAssign(Expr other) const;
  Expr SubAssign(Expr other) const;
  Expr DivAssign(Expr other) const;
  Expr MulAssign(Expr other) const;

  //! Create an int32 Expr.
  explicit Expr(int32_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 32), x); }
  //! Create an int64 Expr.
  explicit Expr(int64_t x) { ptr_ = IntImm::make(Type(type_code_t::Int, 64), x); }
  //! Create an float Expr.
  explicit Expr(float x) { ptr_ = FloatImm::make(Type(type_code_t::Float, 32), x); }
  explicit Expr(bool x) { ptr_ = BoolImm::make(x); }

  //! Construct from dimentions, and get a Tensor.
  explicit Expr(const std::vector<Constant>& dims, primitive_t ptype, const std::string& name = "");

  //! Assign self with another expression.
  //! NOTE Avoid using operator=, it is overloaded in op_overload.h, and will create an Assign Op.
  void Reset(const Expr& other) { set_ptr(other.ptr()); }

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

  bool Equal(const Expr& other) {
    if (ptr_ == other.ptr_) return true;
  }

  primitive_t ptype() const { return ptr_->ptype(); }
  void set_ptype(primitive_t type) { ptr_->set_ptype(type); }

  composite_t ctype() const { return ptr_->ctype(); }
  void set_ctype(composite_t type) { ptr_->set_ctype(type); }

  bool is_unk() const { return ptr_->is_unk(); }
  bool is_boolean() const { return ptr_->is_boolean(); }

  /**
   * Assign an Expr from other or create an ir::Assign node representing the assignment of an ir::Reference node.
   *
   * It has two cases:
   *
   * 1. Expr assign, e.g. `Expr C = A`.
   * 2. Reference assignment, e.g. `C(i,j) = A(i,j) + 1`.
   */
  Expr operator=(const Expr& other);
  Expr operator+=(const Expr& other);
  Expr operator-=(const Expr& other);
  Expr operator*=(const Expr& other);
  Expr operator/=(const Expr& other);

  //! Check whether this Expr is valid for use.
  bool valid() const { return ptr_.get(); }

  //! Tell whether this expression is a operator.
  bool is_op() const;

  bool is_assign_derived() const {
    return type() == ir::NodeTy::Assign || type() == ir::NodeTy::SumAssign || type() == ir::NodeTy::SubAssign ||
           type() == ir::NodeTy::MulAssign || type() == ir::NodeTy::DivAssign;
  }

#define IS_TYPE(m__, ty__) \
  bool is_##m__() const { return type() == ir::NodeTy::ty__; }
  IS_TYPE(var, Var)
  IS_TYPE(allocate, Allocate)
  IS_TYPE(reference, Reference)
  IS_TYPE(assign, Assign)
  IS_TYPE(sum_assign, SumAssign)
  IS_TYPE(sub_assign, SubAssign)
  IS_TYPE(mul_assign, MulAssign)
  IS_TYPE(div_assign, DivAssign)
  IS_TYPE(function_call, Call)
  IS_TYPE(tensor, Tensor)
  IS_TYPE(function, Function)
  IS_TYPE(block, Block)
  IS_TYPE(mark, Mark)
  IS_TYPE(simd_opr, SIMDOpr)
  IS_TYPE(buffer_opr, BufferOpr)
  IS_TYPE(array, Array)
  IS_TYPE(for_, For)
  IS_TYPE(if_then_else, IfThenElse)
  IS_TYPE(le, LE)
  IS_TYPE(lt, LT)
  IS_TYPE(eq, EQ)

  IS_TYPE(int_imm, IntImm);
  IS_TYPE(float_imm, FloatImm);

  IS_TYPE(add, Add);
  IS_TYPE(sub, Sub);
  IS_TYPE(mul, Mul);
  IS_TYPE(div, Div);

  IS_TYPE(module, Module);
#undef IS_TYPE

  // Inference the dimention indice on the id-th dimention.
  void InferenceIteratorDomain();
};

/**
 * BufferOpr is the set of operations on Buffer, we combine multiple operations into this single IR node.
 * A buffer created by BufferOpr is a continuous memory allocated in heap.
 */
struct BufferOpr : public ExprNode<BufferOpr> {
  //! The supported operations.
  enum class Opr {
    //! Creation of a buffer.
    kCreate = 0,
    //! Destroy a buffer.
    kDestroy,
    //! Reference a buffer.
    kReference,
    //! Create and assign some data.
    kCreateAssign,
  };

  //! Destination of this buffer.
  Target target;
  //! Size of buffer.
  Expr size;
  //! Operation this node represents.
  Opr operation;
  //! Name of this buffer.
  std::string name;
  //! Hold the data if this opr is kAssign.
  Any assigned_data;

  static Expr make(Target target, Expr size, Opr operation, primitive_t type, const std::string& name = "");

  bool is_create() const { return operation == Opr::kCreate; }
  bool is_destroy() const { return operation == Opr::kDestroy; }
  bool is_reference() const { return operation == Opr::kReference; }
  bool is_create_assign() const { return operation == Opr::kCreateAssign; }

  static const NodeTy node_type = NodeTy::BufferOpr;
};

/**
 * Array is similar to array in C/C++ language, it is a continuous memory allocated in stack. It should keep small.
 */
struct Array : public ExprNode<Array> {
  //! Size of the array, different from Buffer, the size should be constant.
  Expr size;
  std::string name;

  static Expr make(Expr size, primitive_t ptype, const std::string& name = "");

  static bool CheckExprIsConstant(Expr e) {
    // TODO(Superjomn) Implement it latter.
    return true;
  }

  static const NodeTy node_type = NodeTy::Array;
};

/**
 * Reference is the reference of an element of an Expr.
 *
 * For example:
 *
 *     Expr A;
 *     Var i("i");
 *     Var j("i");
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

  //! Extract intervals from the reference of all the dimention iterators.
  std::vector<interval_tuple_t> ExtractIntervals();

  //! Create a Reference Expr.
  static Expr make(Expr expr, const std::vector<Expr>& iterators);

  static const NodeTy node_type = NodeTy::Reference;

 private:
  void InferenceDimsFromIterators();

  void InferenceIteratorDims();
};

class Function : public ExprNode<Function> {
  std::string name_;

 public:
  //! For inline function to expand the definition inplace.
  std::vector<ir::Expr> inputs;

  //! For inline function to expand the definition inplace.
  std::vector<ir::Expr> outputs;

  ir::Expr body;

  static Expr make(const std::string& name,
                   const std::vector<ir::Expr>& inputs,
                   const std::vector<Expr>& outputs,
                   const Expr& body) {
    auto node = std::make_shared<Function>();
    node->name_ = name;
    node->inputs = inputs;
    node->outputs = outputs;
    node->body = body;
    return Expr(node);
  }

  const std::string& name() const { return name_; }

  static const NodeTy node_type = NodeTy::Function;
};

/**
 * Tensor is a placeholder for the inputs of the whole program.
 *
 * NOTE Tensor is discarded now, use Expr({M,N}) instead.
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

  static Expr make(const std::vector<Constant>& dims, primitive_t type, const std::string& name);

  static const NodeTy node_type = NodeTy::Tensor;
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

  static const NodeTy node_type = NodeTy::Sub;
};

struct Mul : public ExprNode<Mul> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Mul;
};

struct Div : public ExprNode<Div> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Div;
};

struct Mod : public ExprNode<Mod> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Mod;
};

struct Min : public ExprNode<Min> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Min;
};

struct Max : public ExprNode<Max> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Max;
};

struct Minus : public ExprNode<Minus> {
  Expr a;

  static Expr make(Expr a);

  static const NodeTy node_type = NodeTy::Minus;
};

struct Exp : public ExprNode<Exp> {
  Expr a;

  static Expr make(Expr a);

  static const NodeTy node_type = NodeTy::Exp;
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

struct LE : public ExprNode<LE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::LE;
};

struct LT : public ExprNode<LT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::LT;
};

struct GT : public ExprNode<GT> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::GT;
};

struct GE : public ExprNode<GE> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::GE;
};

struct And : public ExprNode<And> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::And;
};

struct Or : public ExprNode<Or> {
  Expr a, b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Or;
};

struct Not : public ExprNode<Not> {
  Expr a;

  static Expr make(Expr a) {
    auto node = std::make_shared<Not>();
    node->a = a;
    return Expr(node);
  }

  static const NodeTy node_type = NodeTy::Not;
};

// Block of code.
struct Block : public ExprNode<Block> {
  // statements(Expr) in the block.
  std::vector<Expr> body;

  static Expr make(std::vector<Expr>&& list);

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

  static const NodeTy node_type = NodeTy::IfThenElse;
};

struct For : public ExprNode<For> {
  Expr iter_init, iter_cond, iter_inc;
  Expr body;
  Var iterator;

  static Expr make(Expr iter_init, Expr iter_cond, Expr iter_inc, Expr body, Var iterator);

  static const NodeTy node_type = NodeTy::For;
};

struct Call : public ExprNode<Call> {
  std::vector<Expr> arguments;
  std::string caller;

  static Expr make(const std::string& caller, std::vector<Expr> arguments);

  static const NodeTy node_type = NodeTy::Call;
};

struct Assign : public ExprNode<Assign> {
  Expr a;
  Expr b;

  bool is_store() const { return a.is_reference(); }

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Assign;
};

struct Let : public ExprNode<Let> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::Let;
};

struct SumAssign : public ExprNode<SumAssign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::SumAssign;
};

struct SubAssign : public ExprNode<SubAssign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::SubAssign;
};

struct MulAssign : public ExprNode<MulAssign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::MulAssign;
};

struct DivAssign : public ExprNode<DivAssign> {
  Expr a;
  Expr b;

  static Expr make(Expr a, Expr b);

  static const NodeTy node_type = NodeTy::DivAssign;
};

/**
 * Statement is the statement of CINN IR, it is a special kind of Expr, which not return a value.
 * It is used to make `Stage` more natual to embed in IR.
 */
class Statement : public ExprNode<Statement> {
 public:
  Expr expr;

  Statement() = default;

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

// Math functions.

class Tanh : public ir::ExprNode<Tanh> {
 public:
  Expr a;

  static Expr make(const Expr& e) {
    auto node = std::make_shared<Tanh>();
    node->a = e;
    return Expr(node);
  }

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

  static const NodeTy node_type = NodeTy::Sigmoid;
};

class Mark : public ir::ExprNode<Mark> {
 public:
  std::string content;

  static Expr make(const std::string& content);

  static const NodeTy node_type = NodeTy::Mark;
};

/**
 * Cast a Expr from the original type to the another type.
 *
 * NOTE Cast op will expand the expression inplace.
 */
struct Cast : public ir::ExprNode<Cast> {
  Expr expr;

  static Expr make(Expr expr, primitive_t type, composite_t ctype = composite_t::primitive);

  static bool CheckPTypeCastable(primitive_t s, primitive_t t) {
    // TODO(Superjomn) Implement it latter.
    return true;
  }
  static bool CheckCTypeCastable(composite_t s, composite_t t) {
    // TODO(Superjomn) Implement it latter.
    return true;
  }

  static const NodeTy node_type = NodeTy::Cast;
};

/// Lower IR instructions such as SIMD
struct SIMDOpr : public ir::ExprNode<SIMDOpr> {
  enum class Opr {
    kAdd = 0,
    kSub,
    kMul,
    kDiv,
  };

  int vector_width;
  Opr opr;
  Expr a, b;

  static Expr make(int vector_width, Opr opr, Expr a, Expr b);

  static const NodeTy node_type = NodeTy::SIMDOpr;
};

/**
 * CallOnce is a block that call only once.
 */
struct CallOnce : public ir::ExprNode<CallOnce> {
  std::string cond_var_name;
  Expr block;

  static Expr make(Expr block);

  static const NodeTy node_type = NodeTy::CallOnce;
};

struct Module : public ir::ExprNode<Module> {
  //! The section of global data definitions.
  Expr global_data_section;
  //! The section of functions.
  Expr function_section;

  static Expr make(Expr data_section, Expr function_section);

  static const NodeTy node_type = NodeTy::Module;
};

//! Extract the Vars from a expression.
std::vector<Var> ExtractVarsFromExpr(const Expr& expr);

//! Build a domain considering all the dimensions.
// e.g. [ M, N, K ] {:} will get { [i0, i1, i2] : 0 <= i0 <= M and 0 <= i1 <= i1 and 0 <= i2 <= K }
isl::set BuildDomainFromDimensions(const std::vector<Constant>& dims, const std::vector<std::string>& iter_names);

//! Generate iterator's name.
// e.g. get i2 if id=2
inline std::string GenIndexedIteratorName(int id);

/**
 * Inference the iterator domain from multiple expressions and dimensions.
 * @param exprs iterator expressions from a reference.
 * @param dimensions dimensions of a Tensor.
 * @return a isl set that represents the iterator domain containing all the iterators.
 */
isl::set BuildDomainFromExprWithDimension(const std::vector<Expr>& exprs, const std::vector<Constant>& dimensions);

}  // namespace ir
}  // namespace cinn
