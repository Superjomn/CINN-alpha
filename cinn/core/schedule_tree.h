#pragma once

#include <glog/logging.h>
#include <isl/cpp.h>
#include <memory>
#include <utility>
#include <vector>

namespace cinn {

enum class ScheduleNodeType {
  None,
  Band,
  Context,
  Domain,
  Extension,
  Filter,
  Sequence,
  Set,
  NodeType,
};

class ScheduleTree;
class ScheduleBand;
class ScheduleFilter;

using schedule_tree_ptr_t = std::unique_ptr<ScheduleTree>;

class ScheduleTree {
 public:
  static schedule_tree_ptr_t MakeBand(isl::multi_union_pw_aff mupa, std::vector<schedule_tree_ptr_t>&& children = {});

  static schedule_tree_ptr_t MakeEmptyBand(const ScheduleTree* root);

  static schedule_tree_ptr_t MakeDomain(isl::union_set domain, std::vector<schedule_tree_ptr_t>&& children = {});

  static schedule_tree_ptr_t MakeContext(isl::set context, std::vector<schedule_tree_ptr_t>&& children = {});

  static schedule_tree_ptr_t MakeFilter(isl::union_set filter, std::vector<schedule_tree_ptr_t>&& children = {});

  static schedule_tree_ptr_t MakeScheduleTree(const ScheduleTree& x) { return x.Clone(); }

  template <typename T>
  T* As() {
    static_assert(std::is_base_of<ScheduleTree, T>::value);
    if (type_ != T::type) return nullptr;
    return static_cast<T*>(this);
  }
  template <typename T>
  const T* As() const {
    static_assert(std::is_base_of<ScheduleTree, T>::value);
    if (type_ != T::type) return nullptr;
    return static_cast<const T*>(this);
  }

  std::vector<ScheduleTree*> children();

  std::vector<const ScheduleTree*> children() const;

  void AppendChild(schedule_tree_ptr_t&& child) { children_.emplace_back(std::move(child)); }

  /**
   * Clone the current node.
   */
  virtual schedule_tree_ptr_t Clone() const = 0;

 protected:
  ScheduleTree(isl::ctx ctx, ScheduleNodeType type) : ctx_(ctx), type_(type) {}
  ScheduleTree(isl::ctx ctx, std::vector<std::unique_ptr<ScheduleTree>>&& children, ScheduleNodeType type);
  ScheduleTree(const ScheduleTree& x);

  isl::ctx ctx_;
  std::vector<schedule_tree_ptr_t> children_{};
  ScheduleNodeType type_;
};

class ScheduleContext : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Context;

  static std::unique_ptr<ScheduleContext> Make(isl::set context, std::vector<schedule_tree_ptr_t>&& children = {});
  static std::unique_ptr<ScheduleContext> Make(const ScheduleContext* tree,
                                               std::vector<schedule_tree_ptr_t>&& children = {});
  schedule_tree_ptr_t Clone() const override;

  virtual ~ScheduleContext() = default;

 private:
  ScheduleContext() = delete;
  explicit ScheduleContext(isl::set s) : ScheduleTree(s.ctx(), ScheduleNodeType::Context), context_(s) {}
  ScheduleContext(const ScheduleContext& x) : ScheduleTree(x), context_(x.context_) {}

 private:
  isl::set context_;
};

/**
 * Schedule node domain.
 */
class ScheduleDomain : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Domain;

  static std::unique_ptr<ScheduleDomain> Make(isl::union_set domain, std::vector<schedule_tree_ptr_t>&& children = {});
  static std::unique_ptr<ScheduleDomain> Make(const ScheduleDomain* tree,
                                              std::vector<schedule_tree_ptr_t>&& children = {});
  virtual ~ScheduleDomain() = default;

  schedule_tree_ptr_t Clone() const override;

 private:
  ScheduleDomain() = delete;
  explicit ScheduleDomain(isl::union_set us) : ScheduleTree(us.ctx(), {}, ScheduleNodeType::NodeType), domain_(us) {}
  ScheduleDomain(const ScheduleDomain& x) : ScheduleTree(x), domain_(x.domain_) {}

  isl::union_set domain_;

  friend class ScheduleTree;
};

/**
 * Schedule node filter.
 */
class ScheduleFilter : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Filter;

  static std::unique_ptr<ScheduleFilter> Make(isl::union_set filter, std::vector<schedule_tree_ptr_t>&& children = {});
  static std::unique_ptr<ScheduleFilter> Make(const ScheduleFilter* tree,
                                              std::vector<schedule_tree_ptr_t>&& children = {});

  schedule_tree_ptr_t Clone() const override { return Make(this); }

  virtual ~ScheduleFilter() = default;

 private:
  ScheduleFilter() = delete;
  explicit ScheduleFilter(isl::union_set s) : ScheduleTree(s.ctx(), {}, ScheduleNodeType::NodeType), filter_(s) {}
  ScheduleFilter(const ScheduleFilter& x) : ScheduleTree(x), filter_(x.filter_) {}

  isl::union_set filter_;
};

/**
 * Schedule node sequence.
 */
class ScheduleSequence : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Sequence;

  static std::unique_ptr<ScheduleSequence> Make(isl::ctx ctx, std::vector<schedule_tree_ptr_t>&& children = {});
  static std::unique_ptr<ScheduleSequence> Make(const ScheduleSequence* tree,
                                                std::vector<schedule_tree_ptr_t>&& children = {});

  schedule_tree_ptr_t Clone() const override { return Make(this); }

  virtual ~ScheduleSequence() = default;

 private:
  explicit ScheduleSequence(isl::ctx ctx) : ScheduleTree(ctx, {}, ScheduleNodeType::NodeType) {}
  ScheduleSequence(const ScheduleSequence& x) : ScheduleTree(x) {}
};

/**
 * Schedule node set.
 */
class ScheduleSet : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Sequence;

  schedule_tree_ptr_t Clone() const override { return Make(this); }

  virtual ~ScheduleSet() = default;

 private:
  explicit ScheduleSet(isl::ctx ctx) : ScheduleTree(ctx, {}, ScheduleNodeType::NodeType) {}
  ScheduleSet(const ScheduleSet& x) : ScheduleTree(x) {}

 public:
  static std::unique_ptr<ScheduleSet> Make(isl::ctx ctx, std::vector<schedule_tree_ptr_t>&& children = {});
  static std::unique_ptr<ScheduleSet> Make(const ScheduleSet* tree, std::vector<schedule_tree_ptr_t>&& children = {});
};

/**
 * ISL schedule_node_band
 */
class ScheduleBand : public ScheduleTree {
 public:
  static constexpr ScheduleNodeType type = ScheduleNodeType::Band;

  static std::unique_ptr<ScheduleBand> Make(isl::multi_union_pw_aff mupa,
                                            bool permutable,
                                            const std::vector<bool>& coincident,
                                            const std::vector<bool>& unroll,
                                            std::vector<schedule_tree_ptr_t>&& children = {});

  static std::unique_ptr<ScheduleBand> Make(const ScheduleBand* tree, std::vector<schedule_tree_ptr_t>&& children = {});

  schedule_tree_ptr_t Clone() const override { return Make(this); }

  virtual ~ScheduleBand() = default;

 private:
  explicit ScheduleBand(isl::ctx ctx) : ScheduleTree(ctx, {}, ScheduleNodeType::None) {}
  ScheduleBand(const ScheduleBand& x)
      : ScheduleTree(x), permutable_(x.permutable_), mupa_(x.mupa_), coincident_(x.coincident_), unroll_(x.unroll_) {}

  /**
   * @return the number of scheduling dimensions in the band.
   */
  size_t n_member() const;

 private:
  bool permutable_;
  isl::multi_union_pw_aff mupa_;
  std::vector<bool> coincident_;
  std::vector<bool> unroll_;
};

}  // namespace cinn
