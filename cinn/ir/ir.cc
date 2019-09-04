#include "cinn/ir/ir.h"
#include "ir.h"

#include <glog/logging.h>
#include <memory>
#include <set>
#include <utility>
#include "cinn/utils/macros.h"

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<Add>();
  node->a = a;
  node->b = b;
  return Expr(node);
}

//-------------------- Logical expressions -------------------------
Expr EQ::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<EQ>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr NE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<NE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr For::make(Expr iter_init, Expr iter_cond, Expr iter_inc, Expr body, Var iterator) {
  CHECK(iter_inc.valid());
  CHECK(iter_cond.valid());
  CHECK(iter_inc.valid());
  CHECK(body.valid());
  auto node = std::make_shared<For>();
  node->iter_init = std::move(iter_init);
  node->iter_cond = std::move(iter_cond);
  node->iter_inc = std::move(iter_inc);
  node->body = std::move(body);
  node->iterator = std::move(iterator);
  return Expr(node);
}

Expr Mod::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Mod>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Sub::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Sub>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Min::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Min>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Max::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Max>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Minus::make(Expr a) {
  CHECK(a.valid());
  auto node = std::make_shared<Minus>();
  node->a = a;
  return Expr(node);
}

size_t Var::counter_ = 0;

template <>
Parameter::Parameter(const std::string &name, int32_t val) : name_(name) {
  type_ = primitive_t::int32;
  int32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Parameter::Parameter(const std::string &name, float val) : name_(name) {
  type_ = primitive_t::float32;
  fp32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Parameter::Parameter(int32_t val) : name_(DefaultUniqueName()) {
  type_ = primitive_t::int32;
  int32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Parameter::Parameter(float val) : name_(DefaultUniqueName()) {
  type_ = primitive_t::float32;
  fp32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
int32_t Parameter::As<int32_t>() const {
  CHECK(primitive_type() == primitive_t::int32);
  return int32_val_;
}
template <>
float Parameter::As<float>() const {
  CHECK(primitive_type() == primitive_t::float32);
  return fp32_val_;
}
template <>
int64_t Parameter::As<int64_t>() const {
  CHECK(primitive_type() == primitive_t::int64);
  return int64_val_;
}

unsigned int Parameter::counter = 0;

std::string Parameter::__str__() const {
  switch (primitive_type()) {
    case primitive_t::float32:
      return std::to_string(As<float>()) + "fp32";
    case primitive_t::int32:
      return std::to_string(As<int32_t>()) + "i32";
    case primitive_t::int64:
      return std::to_string(As<int64_t>()) + "i64";
    default:
      LOG(FATAL) << "not supported type " << static_cast<int>(primitive_type());
  }
  return "Parameter-UNK";
}

Expr Mul::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto node = std::make_shared<Mul>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Div::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Div a not defined";
  CHECK(b.valid()) << "Div b not defined";

  auto x = std::make_shared<Div>();
  x->a = std::move(a);
  x->b = std::move(b);
  return Expr(x);
}

Expr LT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LT>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr LE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr GT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GT>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr GE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GE>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

std::set<std::string> Var::name_set_;

Expr Block::make(std::vector<Expr> &&list) {
  for (auto &v : list) {
    CHECK(v.valid());
  }
  auto node = std::make_shared<Block>();
  node->list = std::move(list);
  return Expr(node);
}

Expr And::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<And>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr Or::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<Or>();
  node->a = std::move(a);
  node->b = std::move(b);
  return Expr(node);
}

Expr IfThenElse::make(Expr condition, Expr true_block) {
  auto node = std::make_shared<IfThenElse>();
  node->condition = condition;
  node->true_block = true_block;
  return Expr(node);
}

Expr IfThenElse::make(Expr condition, Expr true_block, Expr false_block) {
  auto node = std::make_shared<IfThenElse>();
  node->condition = condition;
  node->true_block = true_block;
  node->false_block = false_block;
  return Expr(node);
}

void IfThenElse::Accept(IRVisitor *x) const {
  condition.Accept(x);
  true_block.Accept(x);
  false_block.Accept(x);
}

#define OP_2_ARG_ACCEPT(op__)             \
  void op__::Accept(IRVisitor *x) const { \
    a.Accept(x);                          \
    b.Accept(x);                          \
  };

OP_2_ARGS_FOR_EACH(OP_2_ARG_ACCEPT);

void Minus::Accept(IRVisitor *x) const { a.Accept(x); }

void For::Accept(IRVisitor *x) const { LOG(ERROR) << "get a for"; }

void Block::Accept(IRVisitor *x) const { LOG(ERROR) << "get a block"; }

Var::operator Expr() {
  auto node = std::make_shared<Var>(name_, primitive_type_, interval_.lower_bound(), interval_.upper_bound());
  return Expr(node);
}

bool Var::CheckNameValid(const std::string &name) {
  if (!name_set_.count(name)) {
    name_set_.insert(name);
    return true;
  }
  return false;
}

Var::Var(const std::string &name, int32_t lower_bound, int32_t upper_bound)
    : name_(name), interval_(lower_bound, upper_bound) {
  CheckNameValid(name);
}

Expr Call::make(const std::string &caller, std::vector<Expr> arguments) {
  for (auto &v : arguments) {
    CHECK(v.valid());
  }

  auto node = std::make_shared<Call>();
  node->caller = caller;
  node->arguments = arguments;
  return Expr(node);
}

Expr Reference::make(Expr expr, const std::vector<Var> &iters) {
  auto x = std::make_shared<Reference>();
  x->target = expr;
  x->iterators = iters;
  return Expr(x);
}

Expr Expr::operator=(const Expr &other) {
  if (!valid() || type() != NodeTy::Reference) {
    ptr_ = other.ptr_;
  } else {
    CHECK(other.valid());
    auto assign = Assign::make(*this, other);
    // reset the pointer
    this->ptr_ = assign.ptr_;
  }
  return *this;
}

Expr Assign::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Assign>();
  node->a = a;
  node->b = b;
  return Expr(node);
}

Expr Expr::operator()(std::vector<Var> iters) {
  this->ptr_ = Reference::make(*this, iters).ptr_;
  return *this;
}

Expr Expr::Assign(Expr other) { return Assign::make(*this, other); }

void Call::Accept(IRVisitor *x) const {}

std::vector<Interval> Reference::ExtractIntervals() {
  CHECK(!iterators.empty()) << "At least one iterator is required";
  std::vector<Interval> intervals;
  for (auto &o : iterators) {
    CHECK(o.interval().lower_bound().primitive_type() == primitive_t::int32)
        << " type is " << static_cast<int>(o.interval().lower_bound().primitive_type());
    CHECK(o.interval().upper_bound().primitive_type() == primitive_t::int32)
        << " type is " << static_cast<int>(o.interval().upper_bound().primitive_type());
    intervals.push_back(o.interval());
  }
  return intervals;
}

}  // namespace ir
}  // namespace cinn
