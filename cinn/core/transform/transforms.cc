#include "cinn/core/transform/transforms.h"
#include "cinn/utils/isl_utils.h"
#include "cinn/utils/logging.h"

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
  LOG_INDENT(0);
  CINN_DEBUG(0) << "tile " << statement_ << " " << iterator_ << " " << tile_size_;

  if (tiled_ || !collected_statements_.count(statement_)) {
    return Visit(node.first_child()).parent();
  }
  auto band = node.as<isl::schedule_node_band>();
  auto partial_schedule = band.get_partial_schedule();

  std::vector<isl::union_pw_aff> union_pw_affs;
  for (int pw_id = 0; pw_id < partial_schedule.size(); pw_id++) {
    auto aff = partial_schedule.at(pw_id);
    CINN_DEBUG(1) << pw_id << "-th aff is " << aff << " space " << aff.space();
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
    } else {
      union_pw_affs.push_back(partial_schedule.at(pw_id));
    }
  }

  // update the partial schedule.
  isl::union_pw_aff_list aff_list(band.ctx(), 0);
  for (int i = 0; i < union_pw_affs.size(); i++) {
    aff_list = aff_list.add(union_pw_affs[i]);
  }
  std::string schedule2_repr = GetStreamStr(aff_list);
  schedule2_repr.front() = '[';
  schedule2_repr.back() = ']';

  isl::multi_union_pw_aff transformed_schedule(partial_schedule.ctx(), schedule2_repr);

  auto new_node = band.insert_partial_schedule(transformed_schedule);
  tiled_ = true;
  return Visit(new_node.first_child()).parent();
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
  CollectFilter(node);
  return Visit(node.first_child()).parent();
}

isl::schedule_node UnrollTransformer::VisitBand(const isl::schedule_node& node) {}

isl::set GetStatementSetFromDomain(const std::string& statement, isl::union_set domain) {
  for (int i = 0; i < isl_union_set_n_set(domain.get()); i++) {
    isl::set set = isl::manage(isl_union_set_get_nth_element(domain.get(), i));
    if (statement == isl_set_get_tuple_name(set.get())) {
      // process set by divide the dimensions by tile_sizes
      return set;
    }
  }
}

int GetStatementSetDimsInDomain(const std::string& statement, isl::union_set domain) {
  auto set = GetStatementSetFromDomain(statement, domain);
  return isl_set_dim(set.get(), isl_dim_set);
}

isl::map GetTileTransform(const std::string& statement, isl::union_set domain, const std::vector<int>& tile_sizes) {
  auto gen_transfrom_from_set = [&](isl::set set) {
    std::string set_statement = isl_set_get_statement_repr(set.get());

    std::vector<std::string> dims;
    std::vector<std::string> conds;
    int ndim = isl_set_n_dim(set.get());
    for (int i = 0; i < ndim; i++) {
      auto dim_name = isl_set_get_dim_name(set.get(), isl_dim_set, i);
      if (i < ndim - tile_sizes.size()) {
        dims.push_back(dim_name);
      } else {
        int tile_size = tile_sizes[i + tile_sizes.size() - ndim];
        dims.push_back(StringFormat("%s1", dim_name));
        conds.push_back(StringFormat("%s1 = floor(%s/%d)", dim_name, dim_name, tile_size));
      }
    }

    auto transform_repr = StringFormat(
        "{ %s -> [%s] : %s }", set_statement.c_str(), Concat(dims, ", ").c_str(), Concat(conds, " and ").c_str());
    LOG(INFO) << "transform_repr " << transform_repr;
    return isl::map(domain.ctx(), transform_repr);
  };

  isl::set statement_set = GetStatementSetFromDomain(statement, domain);
  return gen_transfrom_from_set(statement_set);
}

// isl::union_set isolate2(ctx, "{ isolate[[i1,j1] -> [c,d]] : 32i1 <= 199 and 32j1 <= 199 }");
isl::union_set GetUnrollOption(const std::string& statement, isl::union_set domain, isl::map transform) {
  auto gen_transform = [&](isl::set set) {
    set = set.apply(transform);

    std::vector<std::string> domain_dims;
    std::vector<std::string> range_dims;
    for (int i = 0; i < isl_map_dim(transform.get(), isl_dim_in); i++) {
      domain_dims.push_back("ii" + std::to_string(i));
      range_dims.push_back("jj" + std::to_string(i));
    }

    std::string t2_repr = StringFormat("{ [%s] -> [[%s] -> [%s]] }",
                                       Concat(domain_dims, ", ").c_str(),
                                       Concat(domain_dims, ", ").c_str(),
                                       Concat(range_dims, ", ").c_str());
    isl::map t2(domain.ctx(), t2_repr);
    // t2 = isl::manage(isl_map_set_tuple_name(t2.release(), isl_dim_in, "isolate"));
    LOG(INFO) << "set: " << set;
    LOG(INFO) << "t2: " << t2;
    set = set.apply(t2);
    //"{[i1, j1] -> [[i1,j1]->[a,b]]}");
    LOG(INFO) << "get set " << set;
    set = isl::manage(isl_set_set_tuple_name(set.release(), "isolate"));
    isl::union_set result = isl::manage(isl_union_set_from_set(set.release()));
    return result;
  };

  isl::set statement_set = GetStatementSetFromDomain(statement, domain);
  return gen_transform(statement_set);
}

isl::schedule_node TileTransformer2::VisitBand(const isl::schedule_node& node) {
  LOG_INDENT(0);
  if (tiled_ || !collected_statements_.count(statement_)) return Visit(node.first_child()).parent();

  auto band = node.as<isl::schedule_node_band>();
  auto partial_schedule = band.partial_schedule();
  LOG(INFO) << "partial schedule: " << partial_schedule;

  auto ctx = band.ctx();
  auto vals = isl::manage(isl_multi_val_zero(isl_space_copy(partial_schedule.space().get())));
  vals = vals.add(1);
  for (int i = 0; i < tile_sizes_.size(); i++) vals = vals.set_at(vals.size() - i - 1, tile_sizes_[i]);
  auto new_node = band.tile(vals);

  band = new_node.as<isl::schedule_node_band>();

  isl::union_set domain = isl::manage(isl_schedule_node_get_domain(band.get()));
  auto transform = GetTileTransform(statement_, isl::manage(domain.copy()), tile_sizes_);
  CINN_DEBUG(1) << "domain: " << domain;
  // isl::map transform(ctx, "{ A[i,j] -> [a,b]: a = floor(i/32) and b = floor(j/32)  }");
  CINN_DEBUG(1) << "transform " << transform;
  std::string transform_repr = GetStreamStr(transform);
  auto transformed = domain.apply(transform);
  CHECK_EQ(isl_union_set_n_set(transformed.get()), 1UL);
  isl::set first_element = isl::manage(isl_union_set_get_nth_element(transformed.get(), 0));

  isl::map relation = isl::manage(isl_map_from_domain(first_element.copy()));
  relation = relation.reverse();
  isl::set wrapped = isl::manage(isl_map_wrap(relation.release()));
  wrapped = isl::manage(isl_set_set_tuple_name(wrapped.release(), "isolate"));
  // LOG(INFO) << "relaton: " << wrapped;

  // full tile separation
  new_node = band.set_ast_build_options(wrapped);

  band = new_node.as<isl::schedule_node_band>();
  // unroll
  transform = isl::map(band.ctx(), transform_repr);
  domain = isl::manage(isl_schedule_node_get_domain(band.get()));
  isl::union_set unroll_option = GetUnrollOption(statement_, domain, transform);
  CINN_DEBUG(2) << "unroll_option: " << unroll_option;
  isl::union_set unroll_option2(domain.ctx(), GetStreamStr(unroll_option));
  isl::schedule_node inner_band = new_node.first_child();
  inner_band = inner_band.as<isl::schedule_node_band>().set_ast_build_options(unroll_option2);
  inner_band = inner_band.as<isl::schedule_node_band>().member_set_ast_loop_unroll(
      GetStatementSetDimsInDomain(statement_, domain) - 1);

  tiled_ = true;
  return Visit(inner_band.first_child().parent());
}

isl::schedule_node TransposeTransformer::VisitBand(const isl::schedule_node& node) {
  LOG_INDENT(0);
  if (!collected_statements_.count(statement_)) return Visit(node.first_child());
  CINN_DEBUG(0) << "collected filter " << collected_statements_.size();
  CINN_DEBUG(2) << "get a band";
  CINN_DEBUG(1) << "domain: " << isl::manage(isl_schedule_node_get_domain(node.get()));
  CINN_DEBUG(0) << "partial schedule: " << isl::manage(isl_schedule_node_band_get_partial_schedule(node.get()));
  isl::multi_union_pw_aff partial_schedule = isl::manage(isl_schedule_node_band_get_partial_schedule(node.get()));
  // isl::pw_multi_aff transform(partial_schedule.ctx(), "{ [i,j] -> [j,i] }");
  isl::pw_multi_aff transform = PrepareTransform();
  partial_schedule =
      isl::manage(isl_multi_union_pw_aff_apply_pw_multi_aff(partial_schedule.release(), transform.release()));
  CINN_DEBUG(1) << "transformed partial schedule: " << partial_schedule;

  auto filter_set = collected_statements_[statement_];
  std::string filter_repr = isl_set_get_statement_repr(filter_set.get());
  CINN_DEBUG(1) << "get filtered_set: " << filter_set;

  isl::union_set filter(node.ctx(), "{ A[i,j] }");
  auto new_node = node.insert_partial_schedule(partial_schedule);
  return Visit(new_node.first_child().first_child());
}

void CollectFilter(const isl::schedule_node& node, std::map<std::string, isl::set>* collected_statements) {}

isl::schedule_node TransposeTransformer::VisitFilter(const isl::schedule_node& node) {
  CollectFilter(node);
  return Visit(node.first_child()).parent();
}

}  // namespace cinn
