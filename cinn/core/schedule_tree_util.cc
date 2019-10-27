#include "cinn/core/schedule_tree_util.h"

namespace cinn {

schedule_tree_ptr_t FromIslScheduleNodeBand(isl::schedule_node_band band) {
  auto n = band.n_member();
  std::vector<bool> coincident(n, false);
  std::vector<bool> unroll(n, false);
  for (size_t i = 0; i < n; ++i) {
    coincident[i] = isl_schedule_node_band_member_get_coincident(band.get(), i);
  }
  return ScheduleBand::Make(band.get_partial_schedule(), band.get_permutable(), coincident, unroll);
}

schedule_tree_ptr_t ElemFromIslScheduleNode(isl::schedule_node node);

schedule_tree_ptr_t FromIslScheduleNode(isl::schedule_node node) {
  auto res = ElemFromIslScheduleNode(node);
  size_t n = node.n_children();
  if (n == 1 && node.child(0).isa<isl::schedule_node_leaf>()) return res;
  for (int i = 0; i < n; ++i) {
    res->AppendChild(FromIslScheduleNode(node.child(i)));
  }
  return res;
}

schedule_tree_ptr_t ElemFromIslScheduleNode(isl::schedule_node node) {
  isl::ctx ctx = node.ctx();

  auto band = node.as<isl::schedule_node_band>();
  auto context = node.as<isl::schedule_node_context>();
  auto domain = node.as<isl::schedule_node_domain>();
  auto filter = node.as<isl::schedule_node_filter>();
  auto expansion = node.as<isl::schedule_node_expansion>();
  auto extension = node.as<isl::schedule_node_extension>();
  auto guard = node.as<isl::schedule_node_guard>();
  auto mark = node.as<isl::schedule_node_mark>();

  if (!band.is_null()) {
    return FromIslScheduleNodeBand(band);
  }
  if (!context.is_null()) {
    return ScheduleContext::Make(context.get_context());
  }
  if (!domain.is_null()) {
    return ScheduleDomain::Make(domain.get_domain());
  }
  if (!expansion.is_null()) {
    LOG(FATAL) << "Not implemented";
    return nullptr;
  }
  if (!extension.is_null()) {
    LOG(FATAL) << "Not implemented";
  }
  if (!filter.is_null()) {
    return ScheduleFilter::Make(filter.get_filter());
  }
  if (!guard.is_null()) {
    LOG(FATAL) << "Not implemented";
    return nullptr;
  }
  if (!mark.is_null()) {
    LOG(FATAL) << "Not implemented";
    return nullptr;
  }
  if (node.isa<isl::schedule_node_leaf>()) {
    LOG(FATAL) << "Call on a leaf";
    return nullptr;
  }
  if (node.isa<isl::schedule_node_sequence>()) {
    return ScheduleSequence::Make(ctx);
  }
  if (node.isa<isl::schedule_node_set>()) {
    return ScheduleSet::Make(ctx);
  }
  LOG(FATAL) << "Not implemented from type: " << isl_schedule_node_get_type(node.get());
  return nullptr;
}

}  // namespace cinn
