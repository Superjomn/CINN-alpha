#include "cinn/core/transform/transforms.h"

namespace cinn {

// NOTE tricks here.
bool ContainsDimension(const isl::union_pw_aff& aff, const std::string& dimension) {
  auto aff_repr = GetStreamStr(aff);
  auto range_pos = aff_repr.find("->");
  CHECK_NE(range_pos, std::string::npos);
  auto it = aff_repr.find(dimension);
  return it != std::string::npos;
}

isl::schedule_node TileTransformer::VisitBand(const isl::schedule_node& node) {
  LOG(INFO) << "tile " << statement_ << " " << iterator_ << " " << tile_size_;
  if (!IsBandTileable(node) || tiled_ || statements_filter_collected_.count(statement_) == 0) {
    LOG(INFO) << "skip this band";
    // return GetBase().VisitBand(node);
    return node;
  }
  auto band = node.as<isl::schedule_node_band>();
  auto partial_schedule = band.get_partial_schedule();

  isl::multi_union_pw_aff transformed_schedule = partial_schedule;
  for (int i = 0; i < partial_schedule.size(); i++) {
    auto aff = partial_schedule.at(i);
    LOG(INFO) << i << "-th aff is " << aff;
    if (ContainsDimension(aff, iterator_)) {
      auto transform_repr =
          StringFormat("{ [%s] -> [floor(%s/%d)] }", iterator_.c_str(), iterator_.c_str(), tile_size_);
      auto transform = isl::union_map(band.ctx(), transform_repr);
      auto map = isl::manage(isl_union_map_from_union_pw_aff(aff.release()));
      map = map.apply_range(transform);

      auto aff3 = isl::manage(isl_multi_union_pw_aff_from_union_map(map.release()));
      CHECK_EQ(aff3.size(), 1UL);
      auto final = aff3.get_at(0);
      transformed_schedule = transformed_schedule.set_at(i, final);
      break;
    }
  }

  LOG(INFO) << "transformed schedule: " << transformed_schedule;

  auto new_node = band.insert_partial_schedule(transformed_schedule);

  tiled_ = true;

  // return GetBase().VisitBand(new_node);
  return new_node;
}

bool TileTransformer::IsBandTileable(const isl::schedule_node& node) {
  if (isl_schedule_node_get_type(node.get()) != isl_schedule_node_band) return false;
  if (isl_schedule_node_n_children(node.get()) != 1) return false;
  if (!isl_schedule_node_band_get_permutable(node.get())) return false;

  auto space = isl::manage(isl_schedule_node_band_get_space(node.get()));
  int dims = isl_space_dim(space.get(), isl_dim_set);

  if (dims < 1) return false;
  return IsSimpleInnermostBand(node);
}

bool TileTransformer::IsSimpleInnermostBand(const isl::schedule_node& node) {
  CHECK(isl_schedule_node_get_type(node.get()) == isl_schedule_node_band);
  CHECK_EQ(isl_schedule_node_n_children(node.get()), 1);
  auto child_type = isl_schedule_node_get_type(node.child(0).get());
  if (child_type == isl_schedule_node_leaf) return true;
  if (child_type != isl_schedule_node_sequence) return false;

  auto sequence = node.child(0);

  for (int c = 0, nc = isl_schedule_node_n_children(sequence.get()); c < nc; ++c) {
    auto child = sequence.child(c);
    if (isl_schedule_node_get_type(child.get()) != isl_schedule_node_filter) return false;
    if (isl_schedule_node_get_type(child.child(0).get()) != isl_schedule_node_leaf) return false;
  }
  return true;
}

isl::schedule_node TileTransformer::VisitFilter(const isl::schedule_node& node) {
  statements_filter_collected_.clear();
  isl::union_set filter = isl::manage(isl_schedule_node_filter_get_filter(node.get()));
  auto collect_set = [this](isl::set x) {
    auto name = isl_set_get_tuple_name(x.get());
    statements_filter_collected_[name] = x;
  };
  filter.foreach_set(collect_set);
  return Visit(node.first_child());
}

}  // namespace cinn