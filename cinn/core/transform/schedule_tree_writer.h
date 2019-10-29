#pragma once

#include <glog/logging.h>
#include <isl/cpp.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cinn {

template <typename Derived, typename RetTy = void, typename... Args>
struct ScheduleTreeVisitor {
  Derived& GetDerived() { return *static_cast<Derived*>(this); }
  const Derived& GetDerived() const { return *static_cast<const Derived*>(this); }

  RetTy Visit(const isl::schedule_node& node, Args... args) {
    CHECK(!node.is_null());
    switch (isl_schedule_node_get_type(node.get())) {
      case isl_schedule_node_domain:
        LOG(INFO) << "get domain";
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
        return GetDerived().VisitDomain(node, std::forward<Args>(args)...);
      case isl_schedule_node_band:
        // LOG(INFO) << "get band\n" << node;
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
        return GetDerived().VisitBand(node, std::forward<Args>(args)...);
      case isl_schedule_node_sequence:
        // LOG(INFO) << "get sequence\n" << node;
        CHECK_GE(node.n_children(), 2);
        return GetDerived().VisitSequence(node, std::forward<Args>(args)...);
      case isl_schedule_node_set:
        LOG(INFO) << "get set";
        CHECK_GE(isl_schedule_node_n_children(node.get()), 2);
        return GetDerived().VisitSet(node, std::forward<Args>(args)...);
      case isl_schedule_node_leaf:
        // LOG(INFO) << "get leaf\n" << node;
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 0);
        return GetDerived().VisitLeaf(node, std::forward<Args>(args)...);
      case isl_schedule_node_mark:
        // LOG(INFO) << "get mark\n" << node;
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
        return GetDerived().VisitMark(node, std::forward<Args>(args)...);
      case isl_schedule_node_extension:
        LOG(INFO) << "get extension";
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
        return GetDerived().VisitExtension(node, std::forward<Args>(args)...);
      case isl_schedule_node_filter:
        // LOG(INFO) << "get filter\n" << node;
        CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
        return GetDerived().VisitFilter(node, std::forward<Args>(args)...);
      default:
        LOG(FATAL) << "Unsupported isl schedule node detected";
    }
  }

  RetTy VisitDomain(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitSingleChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitBand(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitSingleChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitSequence(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitMultiChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitSet(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitMultiChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitLeaf(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitNode(node, std::forward<Args>(args)...);
  }
  RetTy VisitMark(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitSingleChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitExtension(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitSingleChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitFilter(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitSingleChild(node, std::forward<Args>(args)...);
  }
  RetTy VisitSingleChild(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitNode(node, std::forward<Args>(args)...);
  }
  RetTy VisitMultiChild(const isl::schedule_node& node, Args... args) {
    return GetDerived().VisitNode(node, std::forward<Args>(args)...);
  }
  RetTy VisitNode(const isl::schedule_node& node, Args... args) { LOG(FATAL) << "Unsupported"; }
};

template <typename Derived, typename RetTy = void, typename... Args>
struct RecursiveScheduleTreeVisitor : public ScheduleTreeVisitor<Derived, RetTy, Args...> {
  using BaseTy = ScheduleTreeVisitor<Derived, RetTy, Args...>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *static_cast<const Derived*>(this); }
  Derived& GetDerived() { return *static_cast<Derived*>(this); }
  const Derived& GetDerived() const { return *static_cast<const Derived*>(this); }

  RetTy Visit(const isl::schedule& schedule, Args... args) {
    return GetDerived().Visit(schedule.get_root(), std::forward<Args>(args)...);
  }

  RetTy Visit(const isl::schedule_node& node, Args... args) {
    return GetBase().Visit(node, std::forward<Args>(args)...);
  }

  RetTy VisitNode(const isl::schedule_node& node, Args... args) {
    int n_childern = node.n_children();
    LOG(INFO) << "** n_child " << n_childern;
    for (int i = 0; i < n_childern; ++i) {
      GetDerived().Visit(node.child(i), std::forward<Args>(args)...);
    }
    return RetTy();
  }
};

template <typename Derived, typename... Args>
struct ScheduleNodeRewriter : public RecursiveScheduleTreeVisitor<Derived, isl::schedule_node, Args...> {
  Derived& GetDerived() { return *static_cast<Derived*>(this); }
  const Derived& GetDerived() const { return *static_cast<const Derived*>(this); }

  isl::schedule_node VisitNode(const isl::schedule_node& node, Args... args) {
    if (!node.has_children()) return node;

    isl::schedule_node it = node.first_child();
    while (true) {
      it = GetDerived().Visit(it, std::forward<Args>(args)...);
      if (!it.has_next_sibling()) break;
      it = it.next_sibling();
    }
    return it.parent();
  }

  void CollectFilter(const isl::schedule_node& node) {
    collected_statements_.clear();
    isl::union_set filter = isl::manage(isl_schedule_node_filter_get_filter(node.get()));
    auto collect_set = [this](isl::set x) {
      auto name = isl_set_get_tuple_name(x.get());
      collected_statements_[name] = x;
    };
    filter.foreach_set(collect_set);
  }

 protected:
  std::map<std::string, isl::set> collected_statements_;
};

struct ApplyASTBuildOptions : public ScheduleNodeRewriter<ApplyASTBuildOptions> {
  using BaseTy = ScheduleNodeRewriter<ApplyASTBuildOptions>;
  BaseTy& GetBase() { return *this; }
  const BaseTy& GetBase() const { return *this; }

  ApplyASTBuildOptions() {}

  isl::schedule VisitSchedule(const isl::schedule& schedule) {
    pos_ = 0;
    auto result = Visit(schedule).get_schedule();
    CHECK(pos_ == ast_build_options_.size());
    return result;
  }

  isl::schedule_node VisitBand(const isl::schedule_node& band) {
    auto partial_schedule = isl::manage(isl_schedule_node_band_get_partial_schedule(band.get()));
    LOG(INFO) << "get a band";
    LOG(INFO) << "partial schedule: " << partial_schedule;
    return Visit(band.first_child());
  }

  isl::schedule_node VisitFilter(const isl::schedule_node& node) {
    statements_.clear();
    isl::union_set filter = isl::manage(isl_schedule_node_filter_get_filter(node.get()));
    LOG(INFO) << "filter schedule: " << filter;
    auto collect_set = [this](isl::set x) {
      auto name = isl_set_get_tuple_name(x.get());
      statements_.insert(name);
      LOG(INFO) << "collect statement: " << name;
    };
    filter.foreach_set(collect_set);
    return Visit(node.first_child());
  }

 private:
  size_t pos_;
  std::vector<isl::union_set> ast_build_options_;
  std::set<std::string> statements_;
};

}  // namespace cinn
