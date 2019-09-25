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
  CHECK(dim_pos >= 0);
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
}  // namespace cinn
