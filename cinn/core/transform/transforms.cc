#include "cinn/core/transform/transforms.h"

namespace cinn {

// NOTE tricks here.
std::string::size_type FindDimension(const isl::union_pw_aff& aff, const std::string& dimension) {
  auto aff_repr = GetStreamStr(aff);
  auto range_pos = aff_repr.find("->");
  CHECK_NE(range_pos, std::string::npos);
  auto pos = aff_repr.find("(" + dimension + ")", range_pos + 2);
  return pos;
}

isl::schedule_node TileTransformer::VisitBand(const isl::schedule_node& node) {
  LOG(INFO) << "tile " << statement_ << " " << iterator_ << " " << tile_size_;
  // if (!IsBandTileable(node) || tiled_ || statements_filter_collected_.count(statement_) == 0) {
  // return node;
  //}

  if (tiled_ || statements_filter_collected_.count(statement_) == 0) {
    return node;
  }
  auto band = node.as<isl::schedule_node_band>();
  auto partial_schedule = band.get_partial_schedule();

  std::vector<isl::union_pw_aff> union_pw_affs;
  for (int pw_id = 0; pw_id < partial_schedule.size(); pw_id++) {
    auto aff = partial_schedule.at(pw_id);
    LOG(INFO) << pw_id << "-th aff is " << aff << " space " << aff.space();
    if (FindDimension(aff, iterator_) != std::string::npos) {
      auto transform_repr = StringFormat("{ [%s] -> [floor(%s/%d)*%d, %s] }",
                                         iterator_.c_str(),
                                         iterator_.c_str(),
                                         tile_size_,
                                         tile_size_,
                                         iterator_.c_str());
      auto transform = isl::union_map(band.ctx(), transform_repr);
      auto map = isl::manage(isl_union_map_from_union_pw_aff(aff.release()));
      map = map.apply_range(transform);

      auto transformed_aff = isl::manage(isl_multi_union_pw_aff_from_union_map(map.release()));
      CHECK_GE(transformed_aff.size(), 1UL);
      for (int i = 0; i < transformed_aff.size(); i++) union_pw_affs.push_back(transformed_aff.get_at(i));
      // transformed_schedule = transformed_schedule.set_at(pw_id, final);
    } else {
      union_pw_affs.push_back(partial_schedule.at(pw_id));
    }
  }

  // update the partial schedule.
  isl::union_pw_aff_list aff_list(band.ctx(), 10);
  for (int i = 0; i < union_pw_affs.size(); i++) {
    aff_list = aff_list.add(union_pw_affs[i]);
  }
  std::string schedule2_repr = GetStreamStr(aff_list);
  schedule2_repr.front() = '[';
  schedule2_repr.back() = ']';

  isl::multi_union_pw_aff transformed_schedule(partial_schedule.ctx(), schedule2_repr);

  auto new_node = band.insert_partial_schedule(transformed_schedule);
  tiled_ = true;
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

isl::schedule_node SkewTransformer::VisitBand(const isl::schedule_node& node) {
  auto partial_schedule = node.get_schedule();
}

}  // namespace cinn
