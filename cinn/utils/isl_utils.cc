#include "cinn/utils/isl_utils.h"
#include <glog/logging.h>
#include <isl/space.h>
#include <string>
#include <vector>
#include "cinn/utils/string.h"
#include "isl_utils.h"

namespace cinn {

std::string isl_map_get_statement_repr(__isl_keep isl_map *map, isl_dim_type type) {
  CHECK(map);
  auto tuple_name = isl_map_get_tuple_name(map, type);
  std::vector<std::string> dims;

  for (int i = 0; i < isl_map_dim(map, type); i++) {
    dims.push_back(isl_map_get_dim_name(map, type, i));
  }
  return StringFormat("%s[%s]", tuple_name, Concat(dims, ", ").c_str());
}

std::string isl_set_get_statement_repr(isl_set *set) {
  CHECK(set);
  auto tuple_name = isl_set_get_tuple_name(set);
  std::vector<std::string> dims;

  for (int i = 0; i < isl_set_dim(set, isl_dim_set); i++) {
    dims.push_back(isl_set_get_dim_name(set, isl_dim_set, i));
  }
  return StringFormat("%s[%s]", tuple_name, Concat(dims, ", ").c_str());
}

int isl_map_get_dim_pos_by_name(isl_map *map, isl_dim_type type, const std::string &name) {
  for (int i = 0; i < isl_map_dim(map, type); i++) {
    if (isl_map_get_dim_name(map, type, i) == name) return i;
  }
  return -1;
}

isl_map *isl_map_set_dim_names(isl_map *map, isl_dim_type type, const std::vector<std::string> &names) {
  CHECK(map);
  CHECK_EQ(isl_map_dim(map, type), names.size());
  for (int i = 0; i < names.size(); i++) {
    map = isl_map_set_dim_name(map, type, i, names[i].c_str());
  }
  return map;
}

namespace isl_utils {

__isl_give

    bool
    isl_set_has_dim_name(isl_set *set, const std::string &name) {
  for (int i = 0; i < isl_set_dim(set, isl_dim_set); i++) {
    if (isl_set_get_dim_name(set, isl_dim_set, i) == name) return true;
  }
  return false;
}

bool isl_map_has_dim_name(isl_map *map, isl_dim_type type, const std::string &name) {
  for (int i = 0; i < isl_map_dim(map, type); i++) {
    if (isl_map_get_dim_name(map, type, i) == name) return true;
  }
  return false;
}

isl_union_map *__isl_give isl_calculate_dependency(__isl_take isl_union_map *s0_reads,
                                                   __isl_take isl_union_map *s0_writes,
                                                   __isl_take isl_union_map *s1_reads,
                                                   __isl_take isl_union_map *s1_writes) {
  isl_union_map *reads = isl_union_map_union(s0_reads, s1_reads);
  isl_union_map *writes = isl_union_map_union(s0_writes, s1_writes);
  isl_union_map *reads_writes = isl_union_map_union(isl_union_map_copy(reads), isl_union_map_copy(writes));
  isl_union_map *deps = isl_union_map_apply_range(reads_writes, isl_union_map_reverse(writes));
  return deps;
}

std::string isl_space_get_statement_repr(isl_space *space) {
  std::stringstream ss;
  ss << isl_space_get_tuple_name(space, isl_dim_set);
  ss << "[";
  std::vector<std::string> dims;
  for (int i = 0; i < isl_space_dim(space, isl_dim_set); i++) {
    dims.push_back(isl_space_get_dim_name(space, isl_dim_set, i));
  }

  ss << Concat(dims, ", ");
  ss << "]";
  return ss.str();
}

}  // namespace isl_utils

isl_map __isl_give *isl_set_to_identity_map(__isl_keep isl_set *set) {
  std::vector<std::string> iterator_names;
  int ndims = isl_set_n_dim(set);
  std::string statement = isl_set_to_statement_repr(set);

  std::stringstream ss;

  ss << "{ " << statement << " -> [";
  for (int i = 0; i < ndims - 1; i++) {
    ss << isl_set_get_dim_name(set, isl_dim_set, i) << ",";
  }
  if (ndims > 1) {
    ss << isl_set_get_dim_name(set, isl_dim_set, ndims - 1);
  }
  ss << "] }";
  std::string repr = ss.str();
  LOG(INFO) << "map str: " << repr;
  return isl_map_read_from_str(isl_set_get_ctx(set), repr.c_str());
}

std::string isl_set_to_statement_repr(isl_set *set) {
  std::stringstream ss;
  ss << isl_set_get_tuple_name(set);
  ss << "[";
  int ndims = isl_set_n_dim(set);
  for (int i = 0; i < ndims - 1; i++) {
    ss << isl_set_get_dim_name(set, isl_dim_set, i) << ",";
  }

  if (ndims > 1) {
    ss << isl_set_get_dim_name(set, isl_dim_set, ndims - 1);
  }
  ss << "]";
  return ss.str();
}

__isl_give isl_map *isl_map_add_dim_and_eq_constraint(__isl_take isl_map *map, int dim_pos, int constant) {
  CHECK(map != NULL);
  CHECK_GE(dim_pos, 0);
  CHECK(dim_pos <= (signed int)isl_map_dim(map, isl_dim_out));

  map = isl_map_insert_dims(map, isl_dim_out, dim_pos, 1);
  map = isl_map_set_tuple_name(map, isl_dim_out, isl_map_get_tuple_name(map, isl_dim_in));

  isl_space *sp = isl_map_get_space(map);
  isl_local_space *lsp = isl_local_space_from_space(isl_space_copy(sp));
  isl_constraint *cst = isl_constraint_alloc_equality(lsp);
  cst = isl_constraint_set_coefficient_si(cst, isl_dim_out, dim_pos, 1);
  cst = isl_constraint_set_constant_si(cst, (-1) * constant);
  map = isl_map_add_constraint(map, cst);

  return map;
}

std::string isl_to_str(isl_set *x) { return isl_set_to_str(x); }
std::string isl_to_str(isl_map *x) { return isl_map_to_str(x); }
std::string isl_to_str(isl_space *x) { return isl_space_to_str(x); }

std::vector<std::string> isl_map_get_dim_names(isl_map *map, isl_dim_type type) {
  CHECK(map);
  std::vector<std::string> result;
  for (int i = 0; i < isl_map_dim(map, type); i++) {
    result.push_back(isl_map_get_dim_name(map, type, i));
  }
  return result;
}

std::string isl_to_str(isl_pw_aff *x) { return isl_union_pw_aff_to_str(isl_union_pw_aff_from_pw_aff(x)); }
std::string isl_to_str(isl_union_pw_aff *x) { return isl_union_pw_aff_to_str(x); }

isl_ast_build *isl_ast_build_set_iterators(__isl_take isl_ast_build *build, const std::vector<std::string> &iterators) {
  CHECK(build);
  CHECK(!iterators.empty());
  auto *ctx = isl_ast_build_get_ctx(build);
  isl_id_list *ids = isl_id_list_alloc(ctx, iterators.size());
  for (int i = 0; i < iterators.size(); i++) {
    isl_id *id = isl_id_alloc(ctx, iterators[i].c_str(), nullptr);
    ids = isl_id_list_add(ids, id);
  }
  return isl_ast_build_set_iterators(build, ids);
}

std::string UpdateGlobalFilter(isl_schedule_node *node) {
  CHECK(node);
  CHECK(isl_schedule_node_get_type(node) == isl_schedule_node_band);
  auto *parent = isl_schedule_node_parent(isl_schedule_node_copy(node));
  switch (isl_schedule_node_get_type(parent)) {
    case isl_schedule_node_filter: {
      LOG(INFO) << "get a filter";
      isl::union_set filter = isl::manage(isl_schedule_node_filter_get_filter(parent));
      isl_set_list *set_list = isl_union_set_get_set_list(filter.get());
      if (isl_set_list_size(set_list) == 1) {
        isl::set first = isl::manage(isl_set_list_get_at(set_list, 0));
        auto tuple_name = isl_set_get_tuple_name(first.get());
        LOG(INFO) << "collect filter tuple " << tuple_name;
        IslTileGenerator::Global().set_schedule_filter(first);
      }
      isl_set_list_free(set_list);
    } break;
    case isl_schedule_node_domain: {
      LOG(INFO) << "get a domain";
      isl::union_set filter = isl::manage(isl_schedule_node_domain_get_domain(parent));
      isl_set_list *set_list = isl_union_set_get_set_list(filter.get());
      if (isl_set_list_size(set_list) == 1) {
        isl::set first = isl::manage(isl_set_list_get_at(set_list, 0));
        auto tuple_name = isl_set_get_tuple_name(first.get());
        LOG(INFO) << "collect filter tuple " << tuple_name;
        IslTileGenerator::Global().set_schedule_filter(first);
      }
      isl_set_list_free(set_list);
    } break;

    default:
      break;
  }
}

isl_schedule_node *node_tiler(isl_schedule_node *node, void *user) {
  CHECK(node);
  auto ntype = isl_schedule_node_get_type(node);
  // value to be returned
  switch (ntype) {
    case isl_schedule_node_band: {
      LOG(INFO) << "xx get band";
      LOG(INFO) << isl_schedule_node_to_str(node);
      if (isl_schedule_node_has_children(node) != isl_bool_true) return node;
      isl_schedule_node *tiled_node;
      isl_space *space = isl_schedule_node_band_get_space(node);
      LOG(INFO) << "latest filter: " << IslTileGenerator::Global().schedule_filter_tuple();
      LOG(INFO) << "target stage: " << IslTileGenerator::Global().stage_name();
      LOG(INFO) << "space: " << isl_space_to_str(space);
      // LOG(INFO) << "relation: " <<
      // isl_union_map_to_str(isl_schedule_node_band_get_partial_schedule_union_map(node));
      // std::string node_name = isl_space_get_tuple_name(space, isl_dim_in);

      UpdateGlobalFilter(node);
      CHECK(node);

      // check whether the node name matches the target stage's name.
      if (!IslTileGenerator::Global().schedule_filter_match_stage()) {
        LOG(INFO) << "skip unmatched stage: " << IslTileGenerator::Global().schedule_filter_tuple();
        return node;
      }
      LOG(INFO) << "get matched node: " << IslTileGenerator::Global().stage_name();
      // This is the only way I found to initialize correctly
      // a multi_val
      // tile_sizes = tile_sizes.add(32);
      auto tile_sizes = IslTileGenerator::Global().GetTileSizes(space);
      LOG(INFO) << "tile_size: " << tile_sizes;

      tiled_node = tile_band(node, tile_sizes.release());
      CHECK(tiled_node);
      // tiled_node = isl_schedule_node_band_set_ast_build_options(tiled_node, isl_union_set_read_from_str(ctx,
      // "{unroll[x]}"));

      // Reset the global schedule_filter_tuple to avoid duplicate process.
      IslTileGenerator::Global().ResetScheduleFilter();
      return tiled_node;
    }

    default:
      return node;
  }
  return node;
}

isl_schedule_node *tile_band(isl_schedule_node *node, isl_multi_val *sizes) {
  CHECK(isl_schedule_node_get_type(node) == isl_schedule_node_band);
  isl_ctx *ctx = isl_schedule_node_get_ctx(node);
  int scale_tile;
  int shift_point;

  scale_tile = isl_options_get_tile_scale_tile_loops(ctx);
  isl_options_set_tile_scale_tile_loops(ctx, 1);
  shift_point = isl_options_get_tile_shift_point_loops(ctx);
  isl_options_set_tile_shift_point_loops(ctx, 1);

  CHECK(node);
  LOG(INFO) << "space: " << isl_schedule_node_band_get_space(node);
  LOG(INFO) << "sizes: " << isl_multi_val_to_str(sizes);
  node = isl_schedule_node_band_tile(node, sizes);
  CHECK(node);

  isl_options_set_tile_scale_tile_loops(ctx, scale_tile);
  isl_options_set_tile_shift_point_loops(ctx, shift_point);

  return node;
}

isl_set *isl_set_append_cond(isl_set *set, const char *cond) {
  if (!cond) return set;

  std::string set_repr = isl_set_to_str(set);
  isl_ctx *ctx = isl_set_get_ctx(set);

  // ugly append the cond
  set_repr = set_repr.substr(0, set_repr.size() - 2);
  LOG(INFO) << "set_repr: " << set_repr;

  if (set_repr.find("and") == std::string::npos) {
    set_repr = StringFormat("%s %s }", set_repr.c_str(), cond);
  } else {
    set_repr = StringFormat("%s and %s }", set_repr.c_str(), cond);
  }
  LOG(INFO) << "repr " << set_repr;
  return isl_set_read_from_str(ctx, set_repr.c_str());
}

}  // namespace cinn
