#include "cinn/core/schedule_tree.h"

namespace cinn {

std::unique_ptr<ScheduleBand> ScheduleBand::Make(isl::multi_union_pw_aff mupa,
                                                 bool permutable,
                                                 const std::vector<bool> &coincident,
                                                 const std::vector<bool> &unroll,
                                                 std::vector<schedule_tree_ptr_t> &&children) {
  CHECK_EQ(mupa.size(), coincident.size());
  CHECK_EQ(mupa.size(), unroll.size());
  isl::ctx ctx(mupa.ctx());
  std::unique_ptr<ScheduleBand> band(new ScheduleBand(ctx));
  band->permutable_ = permutable;
  band->mupa_ = isl::manage(isl_multi_union_pw_aff_floor(mupa.release()));
  band->coincident_ = coincident;
  band->unroll_ = unroll;
  band->children_ = std::move(children);
  return band;
}

size_t ScheduleBand::n_member() const {
  CHECK(!mupa_.is_null());
  return mupa_.size();
}

std::unique_ptr<ScheduleContext> ScheduleContext::Make(isl::set context, std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleContext> x(new ScheduleContext(context));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleContext> ScheduleContext::Make(const ScheduleContext *tree,
                                                       std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleContext> x(new ScheduleContext(*tree));
  x->children_ = std::move(children);
  return x;
}

schedule_tree_ptr_t ScheduleContext::Clone() const { return Make(this); }

std::unique_ptr<ScheduleDomain> ScheduleDomain::Make(isl::union_set domain,
                                                     std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleDomain> x(new ScheduleDomain(domain));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleDomain> ScheduleDomain::Make(const ScheduleDomain *tree,
                                                     std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleDomain> x(new ScheduleDomain(*tree));
  x->children_ = std::move(children);
  return x;
}
schedule_tree_ptr_t ScheduleDomain::Clone() const { return Make(this); }

std::unique_ptr<ScheduleFilter> ScheduleFilter::Make(isl::union_set filter,
                                                     std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleFilter> x(new ScheduleFilter(filter));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleFilter> ScheduleFilter::Make(const ScheduleFilter *tree,
                                                     std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleFilter> x(new ScheduleFilter(*tree));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleSequence> ScheduleSequence::Make(isl::ctx ctx, std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleSequence> x(new ScheduleSequence(ctx));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleSequence> ScheduleSequence::Make(const ScheduleSequence *tree,
                                                         std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleSequence> x(new ScheduleSequence(*tree));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleSet> ScheduleSet::Make(isl::ctx ctx, std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleSet> x(new ScheduleSet(ctx));
  x->children_ = std::move(children);
  return x;
}

std::unique_ptr<ScheduleSet> ScheduleSet::Make(const ScheduleSet *tree, std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleSet> x(new ScheduleSet(*tree));
  x->children_ = std::move(children);
  return x;
}

schedule_tree_ptr_t ScheduleTree::MakeBand(isl::multi_union_pw_aff mupa, std::vector<schedule_tree_ptr_t> &&children) {
  std::vector<bool> coincident(mupa.size(), false);
  std::vector<bool> unroll(mupa.size(), false);
  return ScheduleBand::Make(mupa, false, coincident, unroll, std::move(children));
}

schedule_tree_ptr_t ScheduleTree::MakeEmptyBand(const ScheduleTree *root) {
  auto domain = root->As<ScheduleDomain>();
  CHECK(domain);
  auto space = isl::manage(isl_space_set_from_params(isl_space_copy(domain->domain_.get_space().get())));
  auto mv = isl::multi_val::zero(space);
  auto zero = isl::manage(isl_multi_union_pw_aff_multi_val_on_domain(domain->domain_.copy(), mv.release()));
  return ScheduleTree::MakeBand(zero);
}

schedule_tree_ptr_t ScheduleTree::MakeDomain(isl::union_set domain, std::vector<schedule_tree_ptr_t> &&children) {
  return ScheduleDomain::Make(domain, std::move(children));
}

schedule_tree_ptr_t ScheduleTree::MakeContext(isl::set context, std::vector<schedule_tree_ptr_t> &&children) {
  return ScheduleContext::Make(context, std::move(children));
}

schedule_tree_ptr_t ScheduleTree::MakeFilter(isl::union_set filter, std::vector<schedule_tree_ptr_t> &&children) {
  return ScheduleFilter::Make(filter, std::move(children));
}

std::vector<ScheduleTree *> ScheduleTree::children() {
  std::vector<ScheduleTree *> res;
  res.reserve(children_.size());
  for (auto &x : children_) {
    res.push_back(x.get());
  }
  return res;
}

std::vector<const ScheduleTree *> ScheduleTree::children() const {
  std::vector<const ScheduleTree *> res;
  for (auto &p : children_) {
    res.push_back(p.get());
  }
  return res;
}

ScheduleTree::ScheduleTree(isl::ctx ctx, std::vector<std::unique_ptr<ScheduleTree>> &&children, ScheduleNodeType type)
    : ctx_(ctx), children_(std::move(children)), type_(type) {}

ScheduleTree::ScheduleTree(const ScheduleTree &x) : ctx_(x.ctx_), children_(), type_(x.type_) {
  children_.reserve(x.children_.size());
  for (const auto &child : x.children_) {
    children_.push_back(ScheduleTree::MakeScheduleTree(*child));
  }
}

std::unique_ptr<ScheduleBand> ScheduleBand::Make(const ScheduleBand *tree,
                                                 std::vector<schedule_tree_ptr_t> &&children) {
  std::unique_ptr<ScheduleBand> x(new ScheduleBand(*tree));
  x->children_ = std::move(children);
  return x;
}

}  // namespace cinn
