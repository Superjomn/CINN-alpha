#include "cinn/ir/ir.h"
#include <glog/logging.h>
#include <algorithm>
#include <memory>
#include <set>
#include <utility>
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"
#include "cinn/utils/macros.h"
#include "cinn/utils/string.h"

namespace cinn {
namespace ir {

Expr Add::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  CHECK(!a.is_unk());
  CHECK(!b.is_unk());
  auto node = std::make_shared<Add>();
  node->a = a;
  node->b = b;

  CHECK_EQ(a.ptype(), b.ptype());
  node->set_ptype(a.ptype());
  return Expr(node);
}

//-------------------- Logical expressions -------------------------
Expr EQ::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  CHECK(!b.is_unk());
  auto node = std::make_shared<EQ>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr NE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<NE>();
  CHECK_EQ(a.ptype(), b.ptype());
  CHECK(!a.is_unk());
  node->a = std::move(a);
  node->b = std::move(b);
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr For::make(Expr iter_init, Expr iter_cond, Expr iter_inc, Expr body, Var iterator) {
  CHECK(iter_inc.valid());
  CHECK(iter_cond.valid());
  CHECK(iter_inc.valid());
  CHECK(body.valid());
  auto node = std::make_shared<For>();
  CHECK(!iter_init.is_unk());
  CHECK(!iter_cond.is_unk());
  CHECK(!iter_inc.is_unk());
  node->iter_init = std::move(iter_init);
  node->iter_cond = std::move(iter_cond);
  node->iter_inc = std::move(iter_inc);
  node->body = std::move(body);
  node->iterator = std::move(iterator);
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Expr Mod::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Mod>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(a.ptype(), b.ptype());
  CHECK(!a.is_unk());
  node->set_ptype(node->a.ptype());
  return Expr(node);
}

Expr Sub::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Sub>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(node->a.ptype());
  return Expr(node);
}

Expr Min::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Min>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(a.ptype());
  return Expr(node);
}

Expr Max::make(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<Max>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(node->a.ptype());
  return Expr(node);
}

Expr Minus::make(Expr a) {
  CHECK(a.valid());
  auto node = std::make_shared<Minus>();
  node->a = a;
  CHECK(!node->a.is_unk());
  node->set_ptype(a.ptype());
  return Expr(node);
}

size_t Var::counter_ = 0;

template <>
Constant::Constant(const std::string &name, int32_t val) : name_(name) {
  set_ptype(primitive_t::int32);
  int32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Constant::Constant(const std::string &name, float val) : name_(name) {
  set_ptype(primitive_t::float32);
  fp32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Constant::Constant(int32_t val) : name_(DefaultUniqueName()) {
  set_ptype(primitive_t::int32);
  int32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
Constant::Constant(float val) : name_(DefaultUniqueName()) {
  set_ptype(primitive_t::float32);
  fp32_val_ = val;
  name_ = NameGenerator::Global().NewParameterName();
}

template <>
int32_t Constant::As<int32_t>() const {
  CHECK_EQ(ptype(), primitive_t::int32);
  return int32_val_;
}
template <>
float Constant::As<float>() const {
  CHECK(ptype() == primitive_t::float32);
  return fp32_val_;
}
template <>
int64_t Constant::As<int64_t>() const {
  CHECK(ptype() == primitive_t::int64);
  return int64_val_;
}

template <>
double Constant::As<double>() const {
  CHECK(ptype() == primitive_t::float64);
  return fp64_val_;
}

unsigned int Constant::counter = 0;

std::string Constant::__str__() const {
  switch (ptype()) {
    case primitive_t::float32:
      return std::to_string(As<float>()) + "fp32";
    case primitive_t::int8:
      return std::to_string(As<int32_t>()) + "i8";
    case primitive_t::int32:
      return std::to_string(As<int32_t>()) + "i32";
    case primitive_t::int64:
      return std::to_string(As<int64_t>()) + "i64";
    default:
      LOG(FATAL) << "not supported type " << static_cast<int>(ptype());
  }
  return "Parameter-UNK";
}

Constant::Constant(const Constant &other) {
  name_ = other.name_;
  set_ptype(other.ptype());
  switch (ptype()) {
    case primitive_t::int8:
      int8_val_ = other.int8_val_;
      break;
    case primitive_t::int32:
      int32_val_ = other.int32_val_;
      break;
    case primitive_t::int64:
      int64_val_ = other.int64_val_;
      break;
    case primitive_t::float32:
      fp32_val_ = other.fp32_val_;
      break;
    case primitive_t::float64:
      fp64_val_ = other.fp64_val_;
      break;
    case primitive_t::unk:
      break;
    default:
      LOG(FATAL) << "unsupported type " << static_cast<int>(ptype());
  }
}

Constant::operator Expr() {
  auto node = std::make_shared<Constant>(*this);
  return Expr(node);
}

bool Constant::operator==(const Constant &other) const {
  // If no value is set, check their name.
  if (!name_.empty() && name_ == other.name_) return true;
  // Check the actual value.
  if (ptype() != other.ptype()) return false;

  switch (ptype()) {
    case primitive_t::float32:
      return As<float_t>() == other.As<float_t>();
    case primitive_t::int32:
      return As<int32_t>() == other.As<int32_t>();
    case primitive_t::int64:
      return As<int64_t>() == other.As<int64_t>();
    case primitive_t::float64:
      return As<double>() == other.As<double>();
    case primitive_t::unk:
      return true;

    default:
      LOG(FATAL) << "unsupported primitive type: " << ptype();
  }
  return false;
}

Expr Mul::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Mul a not defined";
  CHECK(b.valid()) << "Mul b not defined";

  auto node = std::make_shared<Mul>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(node->a.ptype());
  return Expr(node);
}

Expr Div::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Div a not defined";
  CHECK(b.valid()) << "Div b not defined";

  auto node = std::make_shared<Div>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(node->a.ptype());
  return Expr(node);
}

Expr LT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LT>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr LE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<LE>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr GT::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GT>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr GE::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<GE>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(!node->a.is_unk());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

std::set<std::string> Var::name_set_;

Expr Block::make(std::vector<Expr> &&list) {
  for (auto &v : list) {
    CHECK(v.valid());
  }
  auto node = std::make_shared<Block>();
  node->exprs = std::move(list);
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Expr And::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<And>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(node->a.is_boolean());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr Or::make(Expr a, Expr b) {
  CHECK(a.valid()) << "Expr a not defined";
  CHECK(b.valid()) << "Expr b not defined";
  auto node = std::make_shared<Or>();
  node->a = std::move(a);
  node->b = std::move(b);
  CHECK_EQ(node->a.ptype(), node->b.ptype());
  CHECK(node->a.is_boolean());
  node->set_ptype(primitive_t::boolean);
  return Expr(node);
}

Expr IfThenElse::make(Expr condition, Expr true_block) {
  auto node = std::make_shared<IfThenElse>();
  node->condition = condition;
  node->true_block = true_block;
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Expr IfThenElse::make(Expr condition, Expr true_block, Expr false_block) {
  auto node = std::make_shared<IfThenElse>();
  node->condition = condition;
  node->true_block = true_block;
  node->false_block = false_block;
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Var::operator Expr() {
  auto node =
      std::make_shared<Var>(data_->name_, ptype(), data_->interval_.lower_bound(), data_->interval_.upper_bound());
  return Expr(node);
}

bool Var::CheckNameValid(const std::string &name) {
  if (!name_set_.count(name)) {
    name_set_.insert(name);
    return true;
  }
  return false;
}

Var::Var(const std::string &name, int32_t lower_bound, int32_t upper_bound) {
  InitData();
  data_->name_ = name;
  data_->interval_ = Interval(lower_bound, upper_bound);
  set_ptype(primitive_t::int32);
  CheckNameValid(name);
}

Expr Call::make(const std::string &caller, std::vector<Expr> arguments) {
  for (auto &v : arguments) {
    CHECK(v.valid());
    CHECK(!v.is_unk());
  }

  auto node = std::make_shared<Call>();
  node->caller = caller;
  node->arguments = arguments;
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Expr Reference::make(Expr expr, const std::vector<Expr> &iterators) {
  CHECK(expr.valid());
  auto x = std::make_shared<Reference>();
  x->target = expr;
  CHECK(!expr.is_unk());
  for (const Expr &iterator : iterators) {
    CHECK(iterator.valid());
    x->iterators.push_back(iterator);
    CHECK(!iterator.is_unk());
  }

  x->set_ptype(expr.ptype());
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

Expr Expr::operator+=(const Expr &other) {
  if (!valid() || type() != NodeTy::Reference) {
    ptr_ = other.ptr_;
  } else {
    CHECK(other.valid());
    auto assign = SumAssign::make(*this, other);
    // reset the pointer
    this->ptr_ = assign.ptr_;
  }
  return *this;
}

Expr Expr::operator-=(const Expr &other) {
  if (!valid() || type() != NodeTy::Reference) {
    ptr_ = other.ptr_;
  } else {
    CHECK(other.valid());
    auto assign = SubAssign::make(*this, other);
    // reset the pointer
    this->ptr_ = assign.ptr_;
  }
  return *this;
}

Expr Expr::operator*=(const Expr &other) {
  if (!valid() || type() != NodeTy::Reference) {
    ptr_ = other.ptr_;
  } else {
    CHECK(other.valid());
    auto assign = MulAssign::make(*this, other);
    // reset the pointer
    this->ptr_ = assign.ptr_;
  }
  return *this;
}

Expr Expr::operator/=(const Expr &other) {
  if (!valid() || type() != NodeTy::Reference) {
    ptr_ = other.ptr_;
  } else {
    CHECK(other.valid());
    auto assign = DivAssign::make(*this, other);
    // reset the pointer
    this->ptr_ = assign.ptr_;
  }
  return *this;
}

Expr Expr::Assign(Expr other) { return Assign::make(*this, other); }
Expr Expr::SumAssign(Expr other) { return SumAssign::make(*this, other); }
Expr Expr::SubAssign(Expr other) { return SubAssign::make(*this, other); }
Expr Expr::MulAssign(Expr other) { return MulAssign::make(*this, other); }
Expr Expr::DivAssign(Expr other) { return DivAssign::make(*this, other); }

template <typename T>
Expr XAssignMake(Expr a, Expr b) {
  CHECK(a.valid());
  CHECK(b.valid());
  auto node = std::make_shared<T>();
  node->a = a;
  node->b = b;
  CHECK(!node->b.is_unk());
  node->a.set_ptype(node->b.ptype());
  node->set_ptype(node->b.ptype());
  return Expr(node);
}

Expr Assign::make(Expr a, Expr b) { return XAssignMake<Assign>(a, b); }
Expr SumAssign::make(Expr a, Expr b) { return XAssignMake<SumAssign>(a, b); }
Expr MulAssign::make(Expr a, Expr b) { return XAssignMake<MulAssign>(a, b); }
Expr DivAssign::make(Expr a, Expr b) { return XAssignMake<DivAssign>(a, b); }
Expr SubAssign::make(Expr a, Expr b) { return XAssignMake<SubAssign>(a, b); }

Expr Expr::operator[](Expr i) {
  LOG_INDENT(6);
  auto vars = CollectVarsFromExpr(i);
  CHECK_LE(vars.size(), 1UL) << "Currently only support at most one variable in a dimension";

  const bool is_var_iterator = !vars.empty();

  if (is_var_iterator) {
    Var *var = const_cast<Var *>(vars.front());
    if (var->ptype() == primitive_t::unk) var->set_ptype(primitive_t::int32);
  }

  // The reference node is initialized and has at least one iterator, append the new vars.
  if (ptr() && type() == ir::NodeTy::Reference) {
    As<Reference>()->iterators.push_back(i);
    if (is_var_iterator) InferenceIteratorDomain(this);
    return *this;
  }

  auto node = Reference::make(*this, {i});
  Expr result(node);

  if (!vars.empty()) InferenceIteratorDomain(&result);

  return result;
}

bool Expr::is_op() const {
  CHECK(valid());
#define OP_COND(T) type() == NodeTy::T ||
  return valid() && (OP_2_ARGS_FOR_EACH(OP_COND)  //
                     false);
}

class IntervalExtractor : public IRVisitor {
 public:
  IntervalExtractor(std::vector<Reference::interval_tuple_t> *intervals) : intervals_(intervals) {}

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

  void Visit(const Var *op) override {
    CHECK(op->interval().lower_bound().ptype() == primitive_t::int32);
    CHECK(op->interval().upper_bound().ptype() == primitive_t::int32);
    auto it = std::find_if(intervals_->begin(), intervals_->end(), [&](const Reference::interval_tuple_t &x) {
      return std::get<0>(x) == op->name();
    });
    if (it == intervals_->end()) {
      intervals_->push_back(std::make_tuple(op->name(), op->interval()));
      LOG(INFO) << "get interval: " << op->name() << " " << op->interval().__str__();
    }
  }

 private:
  std::vector<Reference::interval_tuple_t> *intervals_;
};

std::vector<Reference::interval_tuple_t> Reference::ExtractIntervals() {
  CHECK(!iterators.empty()) << "At least one iterator is required";
  std::vector<Reference::interval_tuple_t> intervals;
  IntervalExtractor extractor(&intervals);
  for (auto &o : iterators) {
    extractor.Visit(&o);
  }
  return intervals;
}

Expr Allocate::make(const std::string &buffer_name, Expr size, primitive_t dtype) {
  auto node = std::make_shared<Allocate>();
  CHECK_EQ(size.ptype(), primitive_t::int32);
  node->buffer_name = buffer_name;
  node->size = size;
  node->dtype = dtype;
  node->set_ptype(primitive_t::void_);
  return Expr(node);
}

Param::Param(const std::string &name, const std::string &cond) {
  data_ = std::make_shared<Data>();
  data_->name = name;
  data_->cond = cond;
}

const std::string &Param::name() const {
  CHECK(data_);
  return data_->name;
}

const std::string &Param::cond() const {
  CHECK(data_);
  return data_->cond;
}

isl::set Param::GetContext() { return isl::set(isl_utils::global_isl_ctx(), "[" + name() + "]->{:" + cond() + "}"); }

isl::set Param::context() const {
  return isl::set(isl_utils::global_isl_ctx(), StringFormat("[%s]->{:%s}", name().c_str(), cond().c_str()));
}

// TODO(Superjomn) Support complex expression with more than one Var.
void Expr::InferenceIteratorDomain(Expr *expr) {
  LOG_INDENT(4);
  // CINN_DEBUG(3) << "expr: " << ir::Dump(*expr);
  isl::union_set result;
  // extract iterator from the expr.
  if (!expr->is_reference()) return;
  CHECK(expr->is_reference()) << "type is " << expr->type();
  auto *ref = expr->As<Reference>();
  if (!ref->target.is_tensor()) return;
  auto *tensor = ref->target.As<Tensor>();
  CHECK_LE(ref->iterators.size(), tensor->dims().size());
  // CINN_DEBUG(6) << "Reference " << ir::Dump(ref->target) << " has " << ref->iterators.size() << " iterators";
  if (ref->iterators.size() == tensor->dims().size()) {
    ref->domain = BuildDomainFromExprWithDimension(ref->iterators, tensor->dims());
    CINN_DEBUG(3) << "set reference's domain: " << ref->domain;
  }
}

Expr Expr::operator()(const std::vector<Expr> &iters) {
  auto node = std::make_shared<Reference>();
  node->target = *this;
  node->iterators = iters;
  // inference the iterators domain

  return Expr(node);
}

Expr::Expr(const std::vector<Constant> &dims, primitive_t ptype = primitive_t::float32, const std::string &name) {
  for (auto &dim : dims) {
    CHECK(dim.is_integer());
  }

  *this = Tensor::make(dims, ptype, name);
}

std::vector<Var> ExtractVarsFromExpr(const Expr &expr) {
  class Collector : public IRVisitor {
   public:
    void Visit(const Expr *op) override { IRVisitor::Visit(op); }
    void Visit(const Var *op) override { IRVisitor::Visit(op); }
  };
  return std::vector<Var>();
}

isl::set BuildDomainFromDimensions(const std::vector<Constant> &dims, const std::vector<std::string> &iterators) {
  LOG_INDENT(6);
  CHECK(!dims.empty());

  std::vector<std::string> constraints;
  for (size_t i = 0; i < dims.size(); i++) {
    // collect constraints
    CHECK(dims[i].is_integer());
    auto constraint = StringFormat("0<= %s < %d", iterators[i].c_str(), dims[i].As<int32_t>());
    constraints.push_back(constraint);
  }

  std::string repr =
      StringFormat("{ [%s] : %s }", Concat(iterators, ", ").c_str(), Concat(constraints, " and ").c_str());
  CINN_DEBUG(3) << "repr: " << repr;

  isl::set result(isl_utils::global_isl_ctx(), repr);
  CINN_DEBUG(3) << "get domain " << result;

  return result;
}

namespace {
// Replace the expression with the indexed name.
Expr ReplaceVarWithIterator(int id, const Expr &expr) {
  class Visitor : public IRVisitor {
    int id_;
    std::set<std::string> vars_;

   public:
    Visitor(int id) : id_(id) {}

    void Visit(const Expr *op) override { IRVisitor::Visit(op); }

    void Visit(const Var *op) override {
      vars_.insert(op->name());
      CHECK_EQ(vars_.size(), 1UL) << "One dimension should contains only one variable.";
      const_cast<Var *>(op)->set_name(GenIndexedIteratorName(id_));
    }
  };

  Expr copied = CopyExpr(expr);
  Visitor visitor(id);
  visitor.Visit(&expr);

  return copied;
}

}  // namespace

isl::set BuildDomainFromExprWithDimension(const std::vector<Expr> &exprs, const std::vector<Constant> &dims) {
  LOG_INDENT(6);
  CHECK_EQ(exprs.size(), dims.size());

  std::vector<std::string> iterator_vars;
  std::vector<std::string> dim_iters;
  // collect var for each iterator expr.
  for (size_t i = 0; i < exprs.size(); i++) {
    auto vars = CollectVarsFromExpr(exprs[i]);
    std::set<std::string> iter_names;
    for (auto &var : vars) iter_names.insert(var->name());
    CHECK_EQ(iter_names.size(), 1UL);
    std::vector<std::string> sorted(iter_names.begin(), iter_names.end());

    iterator_vars.push_back(sorted.front());
    dim_iters.push_back(GenIndexedIteratorName(i));
  }

  auto domains = BuildDomainFromDimensions(dims, dim_iters);
  isl::union_map ts;

  std::vector<std::string> iterators, targets, alias, alias_eq;

  for (size_t i = 0; i < dims.size(); i++) {
    targets.push_back(ir::Dump(exprs[i]));
    alias.push_back(dim_iters[i]);
    alias_eq.push_back(targets[i] + "=" + dim_iters[i]);
  }

  std::string repr = StringFormat("{ [%s] -> [%s] : %s }",
                                  Concat(alias, ", ").c_str(),
                                  Concat(iterator_vars, ", ").c_str(),
                                  Concat(alias_eq, " and ").c_str());
  CINN_DEBUG(3) << "repr " << repr;
  isl::map transforms(isl_utils::global_isl_ctx(), repr.c_str());

  isl::set result = domains.apply(transforms);

  CINN_DEBUG(3) << "finial domain: " << result;
  return result;
}

std::string GenIndexedIteratorName(int id) { return StringFormat("ii%d", id); }

}  // namespace ir
}  // namespace cinn
