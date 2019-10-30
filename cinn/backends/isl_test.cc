#include <glog/logging.h>
#include <gtest/gtest.h>
#include <isl/aff.h>
#include <isl/ast.h>
#include <isl/ast_build.h>
#include <isl/constraint.h>
#include <isl/cpp.h>
#include <isl/id.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/space.h>
#include <isl/union_set.h>
#include "cinn/utils/isl_utils.h"

std::string isl_to_str(isl_set *x) { return isl_set_to_str(x); }

std::string isl_to_str(isl_map *x) { return isl_map_to_str(x); }

std::string isl_to_str(isl_space *x) { return isl_space_to_str(x); }

struct param_pack_1 {
  int in_dim;
  int out_constant;
};

isl_map *isl_map_add_dim_and_eq_constraint(isl_map *map, int dim_pos, int constant) {
  CHECK(map);
  CHECK_GE(dim_pos, 0);
  CHECK_LE(dim_pos, isl_map_dim(map, isl_dim_out));

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

/**
 * Take a basic map as input, go through all of its constraints,
 * identifies the constraint of the static dimension param_pack_1.in_dim
 * (passed in user) and replace the value of param_pack_1.out_constant if
 * the static dimension is bigger than that value.
 */
isl_stat extract_static_dim_value_from_bmap(__isl_take isl_basic_map *bmap, void *user) {
  struct param_pack_1 *data = (struct param_pack_1 *)user;

  isl_constraint_list *list = isl_basic_map_get_constraint_list(bmap);
  int n_constraints = isl_constraint_list_n_constraint(list);

  for (int i = 0; i < n_constraints; i++) {
    isl_constraint *cst = isl_constraint_list_get_constraint(list, i);
    isl_val *val = isl_constraint_get_coefficient_val(cst, isl_dim_out, data->in_dim);
    // i.e., the coefficient of the dimension data->in_dim is 1
    if (isl_val_is_one(val)) {
      isl_val *val2 = isl_constraint_get_constant_val(cst);
      int const_val = (-1) * isl_val_get_num_si(val2);
      data->out_constant = const_val;
    }
  }

  return isl_stat_ok;
}

// if multiple const values exist, choose the maximal value among them because we
// want to use this value to know by how much we shift the computations back.
// so we need to figure out the maximal const value and use it to shift the iterations
// backward so that that iteration runs before the consumer.
isl_stat extract_constant_value_from_bset(__isl_take isl_basic_set *bset, void *user) {
  struct param_pack_1 *data = (struct param_pack_1 *)user;

  isl_constraint_list *list = isl_basic_set_get_constraint_list(bset);
  int n_constraints = isl_constraint_list_n_constraint(list);

  for (int i = 0; i < n_constraints; i++) {
    isl_constraint *cst = isl_constraint_list_get_constraint(list, i);
    if (isl_constraint_is_equality(cst) && isl_constraint_involves_dims(cst, isl_dim_set, data->in_dim, 1)) {
      isl_val *val = isl_constraint_get_coefficient_val(cst, isl_dim_out, data->in_dim);
      assert(isl_val_is_one(val));
      // assert that the coefficients of all the other dimension spaces are zero.

      isl_val *val2 = isl_constraint_get_constant_val(cst);
      int const_val = (-1) * isl_val_get_num_si(val2);
      data->out_constant = std::max(data->out_constant, const_val);
    }
  }

  return isl_stat_ok;
}

/**
 * Return the value of the static dimension.
 *
 * For example, if we have a map M = {S0[i,j]->[0,0,i,1,j,2]; S0[i,j]->[1,0,i,1,j,3]}
 * and call isl_map_get_static_dim(M, 5, 1), it will return 3.
 */
int isl_map_get_static_dim(isl_map *map, int dim_pos) {
  assert(map != NULL);
  assert(dim_pos >= 0);
  assert(dim_pos <= (signed int)isl_map_dim(map, isl_dim_out));

  struct param_pack_1 *data = (struct param_pack_1 *)malloc(sizeof(struct param_pack_1));
  data->out_constant = 0;
  data->in_dim = dim_pos;

  isl_map_foreach_basic_map(isl_map_copy(map), &extract_static_dim_value_from_bmap, data);

  return data->out_constant;
}

int isl_set_get_const_dim(isl_set *set, int dim_pos) {
  assert(set != NULL);
  assert(dim_pos >= 0);
  assert(dim_pos <= (signed int)isl_set_dim(set, isl_dim_out));

  struct param_pack_1 *data = (struct param_pack_1 *)malloc(sizeof(struct param_pack_1));
  data->out_constant = 0;
  data->in_dim = dim_pos;

  isl_set_foreach_basic_set(isl_set_copy(set), &extract_constant_value_from_bset, data);

  return data->out_constant;
}

/**
 * Set the value \p val for the output dimension \p dim_pos of \p map.
 *
 * Example
 *
 * Assuming the map M = {S[i,j]->[i0,i1,i2]}
 *
 * M = isl_map_set_const_dim(M, 0, 0);
 *
 * Would create the constraint i0=0 and add it to the map.
 * The resulting map is
 *
 * M = {S[i,j]->[i0,i1,i2]: i0=0}
 *
 */
isl_map *isl_map_set_const_dim(isl_map *map, int dim_pos, int val) {
  assert(map != NULL);
  assert(dim_pos >= 0);
  assert(dim_pos <= (signed int)isl_map_dim(map, isl_dim_out));

  isl_map *identity = isl_set_identity(isl_map_range(isl_map_copy(map)));
  // We need to create a universe of the map (i.e., an unconstrained map)
  // because isl_set_identity() create an identity transformation and
  // inserts the constraints that were in the original set.  We don't
  // want to have those constraints.  We want to have a universe map, i.e.,
  // a map without any constraint.
  identity = isl_map_universe(isl_map_get_space(identity));

  isl_space *sp = isl_map_get_space(identity);
  isl_local_space *lsp = isl_local_space_from_space(isl_space_copy(sp));

  // This loops goes through the output dimensions of the map one by one
  // and adds a constraint for each dimension. IF the dimension is dim_pos
  // it add a constraint of equality to val
  // Otherwise it adds a constraint that keeps the original value, i.e.,
  // (output dimension = input dimension)
  // Example
  //  Assuming that dim_pos = 0, val = 10 and the universe map is
  //  {S[i0,i1]->S[j0,j1]}, this loop produces
  //  {S[i0,i1]->S[j0,j1]: j0=0 and j1=i1}
  //  i.e.,
  //  {S[i0,i1]->S[0,i1]}
  for (int i = 0; i < isl_map_dim(identity, isl_dim_out); i++)
    if (i == dim_pos) {
      isl_constraint *cst = isl_constraint_alloc_equality(isl_local_space_copy(lsp));
      cst = isl_constraint_set_coefficient_si(cst, isl_dim_out, dim_pos, 1);
      cst = isl_constraint_set_constant_si(cst, (-1) * (val));
      identity = isl_map_add_constraint(identity, cst);
    } else {
      isl_constraint *cst2 = isl_constraint_alloc_equality(isl_local_space_copy(lsp));
      cst2 = isl_constraint_set_coefficient_si(cst2, isl_dim_in, i, 1);
      cst2 = isl_constraint_set_coefficient_si(cst2, isl_dim_out, i, -1);
      identity = isl_map_add_constraint(identity, cst2);
    }

  map = isl_map_apply_range(map, identity);

  return map;
}

TEST(isl, basic) {
  // Create a new context
  isl_ctx *context = isl_ctx_alloc();
  isl_set *iter_domain = isl_set_read_from_str(context, "[N] -> { A[i,j, z] : 0<=i<100 and 0<=j<100 and 0<=z<=10}");
  LOG(INFO) << "iter_domain " << isl_to_str(iter_domain);
  isl_space *iter_domain_space = isl_set_get_space(iter_domain);
  LOG(INFO) << "isl_space " << isl_to_str(iter_domain_space);
  LOG(INFO) << "space tuple name: " << isl_space_get_tuple_name(iter_domain_space, isl_dim_type::isl_dim_set);
  LOG(INFO) << "num of dims " << isl_set_dim(iter_domain, isl_dim_type::isl_dim_set);
  LOG(INFO) << "num of in dims " << isl_set_dim(iter_domain, isl_dim_type::isl_dim_in);
  LOG(INFO) << "num of out dims " << isl_set_dim(iter_domain, isl_dim_type::isl_dim_out);

  LOG(INFO) << "isl had dim name " << isl_set_has_dim_name(iter_domain, isl_dim_type::isl_dim_set, 0);
  LOG(INFO) << "isl had dim name " << isl_set_has_dim_name(iter_domain, isl_dim_type::isl_dim_set, 1);

  auto *sp = isl_space_map_from_set(iter_domain_space);
  isl_map *sched = isl_map_identity(sp);
  LOG(INFO) << "sp " << isl_to_str(sp);
  LOG(INFO) << "map " << isl_to_str(sched);
  sched = isl_map_coalesce(sched);
  LOG(INFO) << "map " << isl_to_str(sched);
  sched = isl_map_intersect_domain(sched, isl_set_copy(iter_domain));
  LOG(INFO) << "map intersect " << isl_to_str(sched);

  // Add Beta dimensions.
  for (int i = 0; i < isl_space_dim(iter_domain_space, isl_dim_out) + 1; i++) {
    sched = isl_map_add_dim_and_eq_constraint(sched, 2 * i, 0);
    LOG(INFO) << "map " << i << " " << isl_to_str(sched);
  }

  // Add the duplication dimension.
  sched = isl_map_add_dim_and_eq_constraint(sched, 0, 0);
  LOG(INFO) << "map " << isl_to_str(sched);
}

TEST(isl, basic2) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl_set *domain = isl_set_read_from_str(ctx, "[n] -> { A[i] : 0 < i < n }");
  isl_space *space = isl_set_get_space(domain);
  LOG(INFO) << "space: " << isl_space_to_str(space);
  LOG(INFO) << "domain: " << isl_set_to_str(domain);
  // space: [n] -> { [i, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10] }
  space = isl_space_add_dims(space, isl_dim_type::isl_dim_out, 10);
  LOG(INFO) << "space: " << isl_space_to_str(space);
  // domain is not changed.
  LOG(INFO) << "domain: " << isl_set_to_str(domain);

  LOG(INFO) << "domain dim: " << isl_set_get_const_dim(domain, 0);
  LOG(INFO) << "domain dim: " << isl_set_get_const_dim(domain, 1);

  isl_map *transform0 = isl_map_read_from_str(ctx, "{A[i] -> [i+1]}");
  LOG(INFO) << "transform: " << isl_map_to_str(transform0);
  isl_map *scheduled = isl_map_intersect_domain(transform0, domain);
  LOG(INFO) << "schedule: " << isl_map_to_str(scheduled);

  // tuple name
  LOG(INFO) << "tuple name: " << isl_set_get_tuple_name(domain);
  domain = isl_set_set_tuple_name(domain, "S0");
  LOG(INFO) << "tuple name: " << isl_set_get_tuple_name(domain);
  LOG(INFO) << "domain: " << isl_set_to_str(domain);
  isl_id *dim_id = isl_id_alloc(ctx, "ii", nullptr);
  domain = isl_set_set_dim_id(domain, isl_dim_type::isl_dim_out, 0, dim_id);
  LOG(INFO) << "domain: " << isl_set_to_str(domain);
}

TEST(isl, basic3) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl_union_set *domain =
      isl_union_set_read_from_str(ctx, "{ S1[i,j]: 0 <= i,j <= 10 and i < j-1; S2[i,j]: 0 <=i,j<=10 and i >j+1}");
  auto *domain1 = isl_union_set_read_from_str(ctx, "{S[i,j ] : 0 < i,j<10}");
  LOG(INFO) << "domain: " << isl_union_set_to_str(domain);
  auto *domain2 = isl_union_set_intersect(domain, domain1);
  LOG(INFO) << "domain2: " << isl_union_set_to_str(domain2);
}

TEST(isl, set1) {
  auto *ctx = isl_ctx_alloc();
  isl::union_set domain(ctx, "[N] -> { S[i,j] : 0 < i < j < N }");
  LOG(INFO) << "space: " << domain.space();
  LOG(INFO) << isl_set_to_str(isl_union_set_extract_set(domain.copy(), domain.space().get()));

  isl::set domain1(ctx, "[N] -> { [i,j,k,z] : 0 < i < N and 0 < j <k < N and 0 <z< N }");
  LOG(INFO) << "split set: " << isl::manage(isl_set_split_dims(domain1.copy(), isl_dim_set, 0, 2));

  isl::union_set d0(ctx, "[N] -> { [i] : 0 < i < N }");
  isl::union_set d1(ctx, "[N] -> { [j] : 0 < j < N-1 }");
  isl::union_set d2(ctx, "[N] -> { [k] : 0 < k < N }");

  LOG(INFO) << "intesect: " << isl::manage(isl_union_set_intersect(d0.copy(), d1.copy()));
  LOG(INFO) << "intesect: " << isl::manage(isl_union_set_union(d0.copy(), d1.copy()));
}

TEST(isl, map_set_tuple_name) {
  isl_ctx *ctx = isl_ctx_alloc();
  auto *schedule = isl_map_read_from_str(ctx, "{S[i] -> T[i]}");
  schedule = isl_map_set_tuple_name(schedule, isl_dim_in, "S1");
  schedule = isl_map_set_tuple_name(schedule, isl_dim_out, "T1");
  LOG(INFO) << "schedule " << isl_map_to_str(schedule);
}

TEST(isl, map_dim_name) {
  isl_ctx *ctx = isl_ctx_alloc();
  auto *schedule = isl_map_read_from_str(ctx, "{S[i] -> T[j, k]}");
  std::string name = isl_map_get_dim_name(schedule, isl_dim_in, 0);
  ASSERT_EQ(name, "i");
  LOG(INFO) << "in dim0 " << name;
  name = isl_map_get_dim_name(schedule, isl_dim_out, 0);
  LOG(INFO) << "out dim0 " << name;
  ASSERT_EQ(name, "j");

  int d = isl_map_find_dim_by_name(schedule, isl_dim_out, "j");
  ASSERT_EQ(d, 0);
  LOG(INFO) << "j offset: " << d;
  d = isl_map_find_dim_by_name(schedule, isl_dim_out, "dim_not_exists");
  LOG(INFO) << "notexist dim offset: " << d;
  ASSERT_EQ(d, -1);

  d = isl_space_dim(isl_map_get_space(schedule), isl_dim_out);
  ASSERT_EQ(d, 2);
  LOG(INFO) << "out dim num: " << d;

  d = isl_space_dim(isl_map_get_space(schedule), isl_dim_in);
  LOG(INFO) << "space: " << isl_space_to_str(isl_map_get_space(schedule));
  ASSERT_EQ(d, 1);

  // Whether the 0th dim has name.
  ASSERT_TRUE(isl_map_has_dim_name(schedule, isl_dim_out, 0) == isl_bool_true);
  std::string dim_name = isl_map_get_dim_name(schedule, isl_dim_out, 0);  // j
  ASSERT_EQ(dim_name, "j");
  dim_name = isl_map_get_dim_name(schedule, isl_dim_out, 1);  // j
  ASSERT_EQ(dim_name, "k");
  schedule = isl_map_set_dim_name(schedule, isl_dim_out, 0, "jj");
  schedule = isl_map_set_dim_name(schedule, isl_dim_out, 1, "kk");
  LOG(INFO) << "schedule after rename out dims: " << isl_map_to_str(schedule);
}

TEST(isl, apply_range) {
  auto *ctx = isl_ctx_alloc();
  auto *t0 = isl_map_read_from_str(ctx, "[N] -> {S[i] -> [i]}");
  auto *t1 = isl_map_read_from_str(ctx, "[N] -> {S[i,ii] -> [i, 0]}");
  auto *t2 = isl_map_read_from_str(ctx, "{[i] -> [i, 0]}");

  auto *t0_ = isl_map_apply_range(isl_map_copy(t0), isl_map_copy(t2));

  LOG(INFO) << "left " << isl_space_get_tuple_name(isl_map_get_space(t0), isl_dim_in);
  LOG(INFO) << "t0_ " << isl_map_to_str(t0_);
}

TEST(isl, code_gen_basic) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));
  LOG(INFO) << "T: " << isl_union_map_to_str(T);

  auto *builder = isl_ast_build_from_context(context);
  auto *ast = isl_ast_build_node_from_schedule_map(builder, T);

  // isl_printer *p = isl_printer_to_str(ctx);
  // isl_printer_set_output_format(p, 0);
  // isl_printer_print_ast_node(p, ast);
  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);
}

TEST(isl, code_gen) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i+t*10]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));

  auto *builder = isl_ast_build_from_context(context);

  isl_options_set_ast_build_atomic_upper_bound(ctx, 1);
  // isl_options_get_ast_build_exploit_nested_bounds(ctx);
  // isl_options_set_ast_build_group_coscheduled(ctx, 1);

  // set iterator names.
  isl_id_list *iterators = isl_id_list_alloc(ctx, 2);
  isl_id *id = isl_id_alloc(ctx, "t", NULL);
  iterators = isl_id_list_add(iterators, id);
  id = isl_id_alloc(ctx, "i", NULL);
  iterators = isl_id_list_add(iterators, id);

  builder = isl_ast_build_set_iterators(builder, iterators);

  auto *ast = isl_ast_build_node_from_schedule_map(builder, T);

  isl_printer *p = isl_printer_to_str(ctx);
  isl_printer_set_output_format(p, 0);
  isl_printer_print_ast_node(p, ast);
  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);
}

TEST(isl, hull) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl_set *set = isl_set_read_from_str(ctx, "[N] -> { A[i] : 0 < i < N }");
  LOG(INFO) << "hull: " << isl_set_to_str(isl_set_from_basic_set(isl_set_affine_hull(set)));
}

/*
 *
  isl_ast_node_for = 1,
  isl_ast_node_if,
  isl_ast_node_block,
  isl_ast_node_mark,
  isl_ast_node_user
 */

void walk(isl_ast_node *body) {
  LOG(INFO) << "walk " << isl_ast_node_get_type(body);
  if (isl_ast_node_get_type(body) == isl_ast_node_block) {
    isl_ast_node_list *list = isl_ast_node_block_get_children(body);

    for (int i = isl_ast_node_list_n_ast_node(list) - 1; i >= 0; i--) {
      isl_ast_node *child = isl_ast_node_list_get_ast_node(list, i);

      LOG(INFO) << "child " << i << " " << isl_ast_node_get_type(child);

      if (isl_ast_node_get_type(child) == isl_ast_node_user) {
        LOG(INFO) << "node_user";
      } else if (isl_ast_node_get_type(child) == isl_ast_node_for) {
        LOG(INFO) << "for";
      } else {
        LOG(INFO) << "other";
      }
    }
  } else if (isl_ast_node_get_type(body) == isl_ast_node_for) {
    isl_ast_expr *iter = isl_ast_node_for_get_iterator(body);
    LOG(INFO) << "iter " << isl_ast_expr_to_C_str(iter);

    body = isl_ast_node_for_get_body(body);
    walk(body);
  } else if (isl_ast_node_get_type(body) == isl_ast_node_user) {
    LOG(INFO) << "get user";
    auto *ast = isl_ast_node_user_get_expr(body);
    LOG(INFO) << "usr expr " << isl_ast_expr_to_C_str(ast);
  }
}

TEST(isl, walk_nodes) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t,i]: 0 <= t < T and 1 <=i < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t,i+t*10]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));

  auto *builder = isl_ast_build_from_context(context);
  auto *ast = isl_ast_build_node_from_schedule_map(builder, T);

  isl_printer *p = isl_printer_to_str(ctx);
  isl_printer_set_output_format(p, 0);
  isl_printer_print_ast_node(p, ast);
  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);

  auto node_type = isl_ast_node_get_type(ast);
  LOG(INFO) << "node type " << node_type;
  ASSERT_EQ(node_type, isl_ast_node_for);

  isl_ast_expr *iter = isl_ast_node_for_get_iterator(ast);
  char *iterator_str = isl_ast_expr_to_C_str(iter);
  LOG(INFO) << "iter " << iterator_str;

  LOG(INFO) << "init " << isl_ast_expr_to_C_str(isl_ast_node_for_get_init(ast));
  LOG(INFO) << "inc " << isl_ast_expr_to_C_str(isl_ast_node_for_get_inc(ast));

  auto *body = isl_ast_node_for_get_body(ast);
  walk(body);
}

TEST(union_map, test) {
  isl_ctx *ctx = isl_ctx_alloc();
  auto *set = isl_union_set_read_from_str(ctx, "{ A[i]: 0 < i < 100; B[i]: 0 < i < 100 }");
  auto *map = isl_union_map_read_from_str(ctx, "{ A[i] -> [i]; B[i] -> [i] }");

  auto *map1 = isl_union_map_intersect_domain(map, set);
  LOG(INFO) << "map1 " << isl_union_map_to_str(map1);
}

TEST(pw_multi_aff, test) {
  auto *ctx = isl_ctx_alloc();
  auto *map = isl_map_read_from_str(ctx, "{ A[i,j] -> [i,j] }");
  auto *pw_multi_aff = isl_pw_multi_aff_from_map(map);
  LOG(INFO) << "pw_multi_aff: " << isl_pw_multi_aff_to_str(pw_multi_aff);

  auto *aff = isl_aff_read_from_str(ctx, "{ [i] -> [i+1] }");
  LOG(INFO) << "aff: " << isl_aff_to_str(aff);

  auto *multi_aff_ = isl_multi_aff_from_aff(aff);
  LOG(INFO) << "multi_aff: " << isl_multi_aff_to_str(multi_aff_);
  multi_aff_ = isl_multi_aff_add(multi_aff_, isl_multi_aff_from_aff(isl_aff_read_from_str(ctx, "{ [j] -> [j*10] }")));
  LOG(INFO) << "multi_aff: " << isl_multi_aff_to_str(multi_aff_);
}

TEST(isl, create_indices) {
  auto *ctx = isl_ctx_alloc();
  isl_union_set *_set = isl_union_set_read_from_str(ctx, "{A[i,j]: 0<i,j<10 }");
  LOG(INFO) << "space: " << isl_space_to_str(isl_union_set_get_space(_set));
  isl_union_map *_schedule = isl_union_map_read_from_str(ctx, "{ S[i, j] -> [0, j, i] }");
  LOG(INFO) << "_schedule " << isl_union_map_to_str(_schedule);
  isl_map *schedule = isl_map_from_union_map(_schedule);
  LOG(INFO) << "schedule " << isl_map_to_str(schedule);

  auto *map = isl_map_reverse(schedule);
  LOG(INFO) << "reversed " << isl_map_to_str(map);

  auto *iterator_map = isl_pw_multi_aff_from_map(map);
  LOG(INFO) << "iter_map " << isl_pw_multi_aff_to_str(iterator_map);

  auto *access = isl_map_read_from_str(ctx, "{ S[i,j] -> Buffer[i, j] }");

  auto *index_aff = isl_pw_multi_aff_from_map(isl_map_copy(access));
  LOG(INFO) << "index_aff " << isl_pw_multi_aff_to_str(index_aff);

  isl_space *model2 = isl_pw_multi_aff_get_space(isl_pw_multi_aff_copy(index_aff));
  LOG(INFO) << "model2 " << isl_space_to_str(model2);

  index_aff = isl_pw_multi_aff_align_params(index_aff, model2);
  LOG(INFO) << "index_aff align_param: " << isl_pw_multi_aff_to_str(index_aff);

  isl_space *model = isl_pw_multi_aff_get_space(isl_pw_multi_aff_copy(index_aff));
  LOG(INFO) << "model: " << isl_space_to_str(model);

  iterator_map = isl_pw_multi_aff_align_params(iterator_map, model);
  LOG(INFO) << "iterator_map align params: " << isl_pw_multi_aff_to_str(iterator_map);

  iterator_map = isl_pw_multi_aff_pullback_pw_multi_aff(index_aff, iterator_map);
  LOG(INFO) << "iterator_map pullback_pw_multi_aff: " << isl_pw_multi_aff_to_str(iterator_map);
}

TEST(isl, cpp) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::set set(ctx, "{ A[i] : 0 < i < 100 }");
  LOG(INFO) << set;
}

TEST(isl, project_out) {
  auto *ctx = isl_ctx_alloc();

  auto *map = isl_map_read_from_str(ctx, "{ A[i,j, t0, t1] -> B[i+1, j+2, 0, 0] }");
  LOG(INFO) << "map: " << isl_map_to_str(map);
  auto *tuple_name = isl_map_get_tuple_name(map, isl_dim_out);
  LOG(INFO) << "tuple name: " << tuple_name;
  auto *map2 = isl_map_project_out(isl_map_copy(map), isl_dim_out, 0, 1);
  map2 = isl_map_set_tuple_name(map2, isl_dim_out, tuple_name);
  LOG(INFO) << "project out, map2: " << isl_map_to_str(map2);
}

TEST(isl, pw_aff) {
  auto *ctx = isl_ctx_alloc();
  isl_pw_aff *aff = isl_pw_aff_read_from_str(ctx, "{ [i] -> [i+1] } ");

  isl_map *map = isl_map_read_from_str(ctx, "{ A[i] -> [i] } ");
  LOG(INFO) << "original map: " << isl_map_to_str(map);

  map = isl_map_apply_range(map, isl_map_from_pw_aff(aff));
  LOG(INFO) << "transformed map: " << isl_map_to_str(map);
}

isl_ast_node *gen(isl_ast_node *node, isl_ast_build *build, void *x) {
  isl::union_map schedule = isl::manage(isl_ast_build_get_schedule(build));
  LOG(INFO) << "schedule " << schedule;
  // LOG(INFO) << "current node: " << isl_ast_node_to_C_str(node);
  LOG(INFO) << "type: " << isl_ast_node_get_type(node);
  if (isl_ast_node_get_type(node) == isl_ast_node_user) {
    auto *expr = isl_ast_node_user_get_expr(node);
    auto *stmt_id = isl_ast_expr_get_id(isl_ast_expr_get_op_arg(expr, 0));
    LOG(INFO) << "stmt_id: " << isl_id_to_str(stmt_id);
    isl::map schedule_map = isl::manage(isl_map_from_union_map(schedule.copy()));
    LOG(INFO) << "schedule_map: " << schedule_map;
    isl::map schedule_map_reverse = isl::manage(schedule_map.copy()).reverse();
    isl::pw_multi_aff iterator_map = isl::manage(isl_pw_multi_aff_from_map(schedule_map_reverse.copy()));
    LOG(INFO) << "iterator_map: " << iterator_map;

    isl::map map = isl::manage(isl_map_from_union_map(schedule.copy()));
    LOG(INFO) << "map: " << map;
    isl::pw_multi_aff iterator_map1 = isl::manage(isl_pw_multi_aff_from_map(map.copy()));
    LOG(INFO) << "iterator_map1 " << iterator_map1;
  }
  return node;
}

TEST(isl, build_get_node_info) {
  auto *ctx = isl_ctx_alloc();
  auto *context = isl_set_read_from_str(ctx, "{:}");
  auto *domain = isl_set_read_from_str(ctx, "[T,N]->{S[t0,t1]: 0 <= t0 < T and 1 <=t1 < N}");
  auto *transform = isl_map_read_from_str(ctx, "[T,N] -> {S[t,i] -> [t-1,i+1]}");
  auto *T = isl_union_map_from_map(isl_map_intersect_domain(transform, domain));
  LOG(INFO) << "T: " << isl_union_map_to_str(T);

  auto *build = isl_ast_build_from_context(context);
  isl_ast_build_set_at_each_domain(build, gen, nullptr);

  auto *ast = isl_ast_build_node_from_schedule_map(build, T);

  LOG(INFO) << "\n" << isl_ast_node_to_C_str(ast);
}

// some basic knowledge.
TEST(isl, space) {
  auto *ctx = isl_ctx_alloc();
  // The parameters and dimentions within the same space are only determined by position.
  isl::space space = isl::manage(isl_space_alloc(ctx, 2, 3, 3));
  LOG(INFO) << "space: " << space;
  // { [p0, p1] -> { [i0, i1, i2] -> [o0, o1, o2] }

  LOG(INFO) << "params: " << space.params();
  // [p0, p1] -> {  :  }

  LOG(INFO) << "domain: " << space.domain();
  // [p0, p1] -> { [i0, i1, i2] }

  isl::set set(ctx, "[N] -> { A[i,j,k] : 0 <= i < j < k < N }");
  LOG(INFO) << set;
  LOG(INFO) << set.space();
  // [N] -> { A[i, j, k] }
  // has only one set.

  isl::map map(ctx, "[N] -> { A[i,j] -> B[i,j] : 0 < i < j < N }");
  LOG(INFO) << map;
  LOG(INFO) << map.space();
  // [N] -> { A[i, j] -> B[o0, o1] }
  // has both in and out set.

  for (int i = 0; i < isl_space_dim(map.space().get(), isl_dim_in); i++)
    LOG(INFO) << "dim " << i << " " << isl_space_get_dim_name(map.space().get(), isl_dim_in, i);

  for (int i = 0; i < isl_space_dim(map.space().get(), isl_dim_param); i++)
    LOG(INFO) << "param " << i << " " << isl_space_get_dim_name(map.space().get(), isl_dim_param, i);

  if (isl_space_has_dim_name(map.space().get(), isl_dim_in, 0)) LOG(INFO) << "space in 0 has dim name";

  ASSERT_EQ(isl_space_find_dim_by_name(map.space().get(), isl_dim_in, "i"), 0);

  map = isl::manage(isl_map_set_tuple_name(map.release(), isl_dim_in, "A'"));
  LOG(INFO) << "rename A to A': " << map;

  // local space
  // ...
}

TEST(isl, set) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl::set set(ctx, "{ A[i,j] : 0 < i < j < 100 }");
  // isl::map map = isl::manage(isl_map_lex_le(set.space().get()));
  // LOG(INFO) << "lex_le: " << map;

  LOG(INFO) << "set: " << set;
  isl::set set1 = isl::manage(isl_set_compute_divs(set.copy()));
  LOG(INFO) << "set1 " << set1;

  isl::map map1(ctx, "{ A[i,j,k] -> B[i',j']: i'=i and j'=j}");
  LOG(INFO) << "map1: " << map1;
  LOG(INFO) << "map1: " << isl_map_to_str(isl_map_compute_divs(map1.copy()));

  // isl_basic_set_list *set_list = isl_set_get_basic_set_list(set.copy());
  // ASSERT_EQ(isl_basic_set_list_n_basic_set(set_list), 1);
  // isl::basic_set set0 = isl::manage(isl_basic_set_list_get_at(set_list, 0));
  // LOG(INFO) << "get 0th basic set: " << set0;

  // constraint
  // LOG(INFO) << "num of constraint: " << isl_basic_set_n_constraint(set0.get());
  // auto *constraint_list = isl_basic_set_get_constraint_list(set0.copy());
  // LOG(INFO) << "constraints: " << isl_constraint_list_to_str(constraint_list);
  // constraints: ({ A[i, j] : -1 + i >= 0 },{ A[i, j] : -1 - i + j >= 0 },{ A[i, j] : 99 - j >= 0 })
  // auto *constraint0 = isl_constraint_list_get_at(constraint_list, 0);
  // if (isl_constraint_is_equality(constraint0)) LOG(INFO) << "constraint 0 is equality";
}

TEST(isl, param) {
  isl_ctx *ctx = isl_ctx_alloc();

  isl::set set(ctx, "{ A[i,j]: 0 < i < j < 100 }");
  LOG(INFO) << "params: " << set.params();
  // params: {  :  }

  isl::set set1(ctx, "[n,m] -> { A[i,j]: n < i < j < m }");
  LOG(INFO) << "set1: " << set1;
  // set1: [n, m] -> { A[i, j] : i > n and i < j < m }
  LOG(INFO) << "params1: " << set1.params();
  // params: [n, m] -> {  : m >= 3 + n }

  auto set1_project_set = isl::manage(isl_set_project_out(set1.copy(), isl_dim_set, 0, 1));
  LOG(INFO) << "set1_project: " << set1_project_set;
  set1_project_set = isl::manage(isl_set_set_tuple_name(set1_project_set.release(), "A1"));
  LOG(INFO) << "set1_project_set add tuple name A1: " << set1_project_set;

  // with more dimensions
  isl::set set2(ctx, "[n,m] -> { A[i,j,k,l,g]: 0 < i < j < n and 0 < k < l < g < m }");
  LOG(INFO) << "set2: " << set2;
  LOG(INFO) << "set2.param: " << set2.params();
  LOG(INFO) << "project(1,2): " << isl::manage(isl_set_project_out(set2.copy(), isl_dim_set, 1, 2));
  // project: [n, m] -> { [i, l, g] : 0 < i <= -2 + n and l >= 2 and l < g < m }
  // project will remove some indices.

  // same is map

  cinn::isl_utils::map map(ctx, "[n,m] -> { A[i,j,k,l,g] -> [i+1,j-1,k*2,l,g] }");
  LOG(INFO) << "map: " << map;
  LOG(INFO) << "map.space: " << map.space();
  // the iterator inside a single map instance is only distiuished by position.
  // map.space: [n, m] -> { A[i, j, k, l, g] -> [o0, o1, o2, o3, o4] }
  LOG(INFO) << "map.project(in, 0, 2): " << map.project_out(isl_dim_in, 0, 2);
  // map.project(in, 0, 2): [n, m] -> { [k, l, g] -> [o0, o1, 2k, l, g] }
  LOG(INFO) << "map.project(out, 0, 2): " << map.project_out(isl_dim_out, 0, 2);
  // map.project(out, 0, 2): [n, m] -> { A[i, j, k, l, g] -> [2k, l, g] }
}

TEST(isl, align_param) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl::set set(ctx, "[n] -> { A[i,j,k]: 0 < i < j < k < n }");
  LOG(INFO) << "set: " << set;
  // set: [n] -> { A[i, j, k] : i > 0 and j > i and j < k < n }

  isl::set set2(ctx, "[n, m, z, N, M] -> { A[k,j] : }");
  isl_space *set2_space = isl_space_copy(set2.space().get());
  isl::set set_aligned_param = isl::manage(isl_set_align_params(set.copy(), set2_space));
  LOG(INFO) << "set aligned param: " << set_aligned_param;
  // [n, m, z, N, M] -> { A[i, j, k] : i > 0 and j > i and j < k < n }

  isl::set set3(ctx, "[] -> { A[k,j] : }");
  isl_space *set3_space = isl_space_copy(set3.space().get());
  set_aligned_param = isl::manage(isl_set_align_params(set.copy(), set3_space));
  LOG(INFO) << "set aligned param: " << set_aligned_param;
  // [n] -> { A[i, j, k] : i > 0 and j > i and j < k < n }
}

TEST(isl, aff) {
  isl_ctx *ctx = isl_ctx_alloc();

  // a quasi-affine expression
  isl::aff aff(ctx, "{ [x,y] -> [floor(2*x + floor(y/4))] }");
  isl::aff aff1(ctx, "{ [x,y] -> [1] }");
  // the -> are omitted if the quasi-affine expression does not have a domain space.
  isl::aff aff2(ctx, "{ [1+2] }");

  LOG(INFO) << "aff: " << aff;
  LOG(INFO) << "aff1: " << aff1;
  LOG(INFO) << "aff2: " << aff2;

  isl::pw_aff pw_aff(ctx, "[N] -> {[x]->[x+1] : 0 <= x <= N; [x]->[0]: x=N}");
  LOG(INFO) << "pw_aff: " << pw_aff;

  isl::pw_aff pw_aff1(ctx, "[N] -> {A[i]->[i] : 0 <= i < N; A[i]->[0]: i=N}");
  LOG(INFO) << "pw_aff: " << pw_aff1;

  // a function
  isl::pw_aff pw_aff2(ctx, "[N] -> {A[i]->[i] : 0 <= i < 2; A[i]->[0]: i=2; A[i]->[2*i]: i<N}");
  LOG(INFO) << "pw_aff: " << pw_aff2;

  // picewise tuple

  isl::pw_multi_aff maff(ctx, "{ [i]->[i,i-1]: i-1>=0 }");
  LOG(INFO) << "multi_aff: " << maff;
  isl::multi_pw_aff maff1 = isl::manage(isl_multi_pw_aff_from_pw_multi_aff(maff.copy()));
  LOG(INFO) << "multi pw aff: " << maff1;
}

// This is a demo from playground.pollylabs.org
TEST(isl, dependence_analysis) {
  // double X[10], Y[10], Z[20];
  // for (int i = 0; i <= 20; ++i)
  // S: Z[i] = 0.;
  // for (int i = 0; i <= 10; ++i)
  //   for (int j = 0; j <= 10; ++j)
  //       T: Z[i + j] += A[i] * B[j];
  //

  isl_ctx *ctx = isl_ctx_alloc();
  isl::union_set domain(ctx, "{A[i] : 0 <= i <= 20; T[i,j]: 0 <= i,j <= 10}");
  isl::union_map schedule(
      ctx, "{S[i] -> [t0, t1, t2]: t0=0 and t1=i and t2=0; T[i,j] -> [t0, t1, t2]: t0=1 and t1=i and t2=j}");
  // access relations
  isl::map A_S_Z(ctx, "{ S[i]->Z[i] : 0 <=i <= 20 }");                     // write
  isl::map A_T_Z(ctx, "{ T[i,j] -> Z[a] : a = i+j and 0 <= i,j <= 10 }");  // write
  isl::map A_T_A(ctx, "{ T[i,j] -> A[i] : 0 <= i,j <= 10 }");              // read
  isl::map A_T_B(ctx, "{ T[i,j] -> B[j] : 0 <= i,j <= 10 }");              // read

  ASSERT_FALSE(A_S_Z.is_null());
  ASSERT_FALSE(A_T_Z.is_null());
  ASSERT_FALSE(A_T_A.is_null());
  ASSERT_FALSE(A_T_B.is_null());

  isl::union_map writes = isl::manage(isl_union_map_add_map(isl_union_map_from_map(A_S_Z.copy()), A_T_Z.copy()));
  isl::union_map reads = isl::manage(isl_union_map_add_map(isl_union_map_from_map(A_T_A.copy()), A_T_B.copy()));
  reads = isl::manage(isl_union_map_add_map(reads.release(), A_T_Z.copy()));

  LOG(INFO) << "reads: " << reads;
  LOG(INFO) << "writes: " << writes;

  // schedule  information
  // lexicographic order.
  isl::space schedule_space = isl::set(ctx, "{ [t0, t1, t2]: }").get_space();
  LOG(INFO) << "schedule_space: " << schedule_space;
  LOG(INFO) << "lex_lt: " << isl::manage(isl_map_lex_lt(schedule_space.copy()));

  isl::union_set schedule_domain = domain.apply(schedule);
  LOG(INFO) << "schedule_domain: " << schedule_domain;
  // schedule_domain: { [0, i1, i2] : 0 <= i1 <= 10 and 0 <= i2 <= 10 }

  // Check S(2) is executed before T(0,0)
  // Solution1: define a map between the two instances in question.
  // Solution2: once in schedule space, check if the relation is a subset of precedes.
  isl::union_map rel = isl::union_map(ctx, "{ S[i] -> T[i,j] }");
  rel = rel.apply_domain(schedule).apply_range(schedule);
  LOG(INFO) << "rel: " << rel;
  // rel: { [0, i1, 0] -> [0, i1, o2] }
  auto precedes = isl::manage(isl_map_lex_lt(schedule_space.copy()));
  LOG(INFO) << "precedes: " << precedes;
  LOG(INFO) << "is precede: " << rel.is_subset(precedes);

  // ...
}

std::string ScheduleGenC(isl_ctx *ctx, const isl::schedule &schedule) {
  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx, "[N, M, K]->{:N = 10 and  M = 20 and K = 30}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  return isl_ast_node_to_C_str(ast);
}

TEST(isl, schedule_tree) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl::union_set domain(ctx, "[N] -> { A[i,j]: 0<=i,j<N; B[i]: 0 <=i < N }");
  {
    LOG(INFO) << "domain: " << domain;
    isl::schedule schedule = isl::manage(isl_schedule_from_domain(domain.copy()));
    LOG(INFO) << "schedule: " << schedule;
    ASSERT_FALSE(schedule.is_null());

    auto root = schedule.get_root();
    LOG(INFO) << "root: " << root;
    ASSERT_EQ(root.get_tree_depth(), 0);

    isl::union_map schedule_map = isl::manage(isl_schedule_get_map(schedule.copy()));
    LOG(INFO) << "schedule_map: " << schedule_map;
    LOG(INFO) << "node type: " << isl_schedule_node_get_type(root.get());

    LOG(INFO) << "schedule tree:";
    cinn::DumpSchedule(schedule);
  }
  {
    LOG(INFO) << "compute schedule";
    isl::union_set domain(ctx,
                          "[N,M,K] -> { S0[i']: 0<=i'<=N; S1[i,j]: 0<=i,j<=N; S2[i,j,k]: 0<=i,j <= M and 0 <= k<=K }");
    // S0 << S1
    isl::union_map validity(ctx, "[N] -> { S0[a] -> S1[b,c]: 0 <=a<=N; S1[b,c]->S2[b,c,k] }");
    isl::union_map proximity(ctx, "[N]->{ S1[a,b]->S2[a,b,k] }");
    isl::schedule_constraints sc = isl::manage(isl_schedule_constraints_on_domain(domain.copy()));
    sc = isl::manage(isl_schedule_constraints_set_validity(sc.release(), validity.copy()));
    sc = isl::manage(isl_schedule_constraints_set_proximity(sc.release(), proximity.copy()));
    isl::schedule schedule = isl::manage(isl_schedule_constraints_compute_schedule(sc.copy()));
    LOG(INFO) << "original schedule: " << cinn::DumpSchedule(schedule);
    LOG(INFO) << "original C code:\n" << ScheduleGenC(ctx, schedule);

    // Add some transformation
    isl::union_map transform(ctx, "[N] -> { S0[i']->S0[i']; S1[i,j]->S1[i,j]; S2[i,j,k] -> S2[j,i,k] }");
    isl::schedule_constraints sc1 = isl::manage(sc.copy());
    sc1 = isl::manage(isl_schedule_constraints_apply(sc1.release(), transform.release()));
    LOG(INFO) << "sc transformed: " << sc1;
    LOG(INFO) << "sc1.coincidence: " << sc1.get_coincidence();
    LOG(INFO) << "sc1.validity: " << sc1.get_validity();
    schedule = isl::manage(isl_schedule_constraints_compute_schedule(sc1.release()));
    LOG(INFO) << "transformed schedule: " << cinn::DumpSchedule(schedule);
    LOG(INFO) << "transformed schedule: " << ScheduleGenC(ctx, schedule);
  }
}

TEST(isl, schedule_tree2) {
  isl_ctx *ctx = isl_ctx_alloc();
  // isl_union_set *domain =
  // isl_union_set_read_from_str(ctx, "[N,M,K] -> { S0[i,j]: 0<i<N and 0<j<M; S1[i,j,k]: 0<i<N and 0<j<M and 0<k<K;
  // S2[i,j]: 0<i<N and 0<j<M }");
  isl_union_set *domain =
      isl_union_set_read_from_str(ctx,
                                  "{ S5[i0, i1] : 0 <= i0 <= 99 and 0 <= i1 <= 199; S3[i0, i1, i2] : 0 <= i0 <= 99 and "
                                  "0 <= i1 <= 199 and 0 <= i2 <= 149; S4[i0, i1] : 0 <= i0 <= 99 and 0 <= i1 <= 199 }");
  // isl_union_map *proximity = isl_union_map_read_from_str(ctx, "[N,M] -> { S0[i,j] -> S1[i,j,k] }");
  // isl_union_map *proximity = isl_union_map_read_from_str(ctx, "{ S3[i0,i1,i2] -> S4[i0,i1]; S4[i0,i1]->S5[i0,i1] }");
  isl_union_map *proximity = isl_union_map_read_from_str(ctx, "{ S4[i0,i1] -> S5[i0,i1] }");

  isl_schedule_constraints *sc = isl_schedule_constraints_on_domain(domain);
  sc = isl_schedule_constraints_set_proximity(sc, proximity);

  isl_schedule *schedule = isl_schedule_constraints_compute_schedule(sc);

  // Dump schedule.
  isl_printer *printer = isl_printer_to_str(ctx);
  printer = isl_printer_set_yaml_style(printer, ISL_YAML_STYLE_BLOCK);
  printer = isl_printer_print_schedule(printer, schedule);
  std::cout << isl_printer_get_str(printer) << std::endl;

  // Dump C like code.
  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx, "[N, M, K]->{:N = 10 and  M = 20 and K = 30}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule);
  std::cout << "C like code:\n" << isl_ast_node_to_C_str(ast) << std::endl;
}

void DisplayScheduleC(isl_schedule *schedule) {
  isl::set C(isl_schedule_get_ctx(schedule), "{:}");
  isl::ast_build build = isl::manage(isl_ast_build_from_context(C.copy()));

  isl::ast_node ast = isl::manage(isl_ast_build_node_from_schedule(build.get(), isl_schedule_copy(schedule)));

  std::cout << "schedule tree get C code:\n\n" << isl_ast_node_to_C_str(ast.get()) << std::endl;
}

TEST(isl, schedule_tile) {
  isl_ctx *ctx = isl_ctx_alloc();

  LOG(INFO) << "compute schedule";
  isl::union_set domain(ctx, "[N,M,K] -> {  S1[i,j]: 0<=i,j<=N; S2[i,j,k]: 0 <i,j,k<N }");
  // S0 << S1
  isl::union_map validity(ctx, "[N] -> {  S1[b,c]->S2[b,c,k] }");
  isl::union_map proximity(ctx, "[N]->{ }");
  isl::schedule_constraints sc = isl::manage(isl_schedule_constraints_on_domain(domain.copy()));
  sc = isl::manage(isl_schedule_constraints_set_validity(sc.release(), validity.copy()));
  sc = isl::manage(isl_schedule_constraints_set_proximity(sc.release(), proximity.copy()));
  isl::schedule schedule = isl::manage(isl_schedule_constraints_compute_schedule(sc.copy()));
  DisplayScheduleC(schedule.get());

  // Add some transformation
  isl::schedule_constraints sc1 = isl::manage(sc.copy());
  schedule = isl::manage(isl_schedule_constraints_compute_schedule(sc1.release()));
  auto *root = isl_schedule_get_root(schedule.get());
  cinn::IslTileGenerator::Global().set_stage_name("S1");
  std::map<std::string, int> tiles;
  tiles["i"] = 32;
  tiles["j"] = 8;
  cinn::IslTileGenerator::Global().set_tiles(tiles);
  DisplayScheduleC(schedule.get());
  LOG(INFO) << "schedule:\n" << cinn::DumpSchedule(schedule);
  root = isl_schedule_node_map_descendant_bottom_up(root, cinn::node_tiler, nullptr);
  schedule = isl::manage(isl_schedule_node_get_schedule(root));
  // LOG(INFO) << "schedule:\n" << cinn::isl_utils::DumpSchedule(schedule.ctx().get(), schedule);

  DisplayScheduleC(schedule.get());
}

TEST(isl, schedule_tree3) {
  std::string schedule_repr = R"ROC(
{ domain: "{ B[i, j] : 0 < i <= 200 and 0 < j <= 200; A[i, j] : 0 < i <= 200 and 0 < j <= 200 }",
  child:
   { sequence:
      [ { filter: "{ A[i, j] }",
          child:
           { schedule: "[{ A[i, j] -> [floor((i)/4)] }, { A[i, j] -> [floor((j)/4)] }, { A[i,j] -> [i] }, { A[i,j] -> [j] }]",
             permutable: 1,
             coincident: [ 1, 1 ],
             options: "{ isolate[[]->[a,b,c,d]] : 4a+4b+3+3<=200 ; [isolate[]->unroll[3]] }"
           } },
        { filter: "{ B[i, j] }",
          child:
           { schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]",
             permutable: 1,
             coincident: [ 1, 1 ] } } ] } }
  )ROC";

  LOG(INFO) << schedule_repr;

  isl::ctx ctx(isl_ctx_alloc());
  isl::schedule schedule = isl::manage(isl_schedule_read_from_str(ctx.get(), schedule_repr.c_str()));

  DisplayScheduleC(schedule.get());
}

TEST(isl, schedule_tree_skew) {
  std::string schedule_repr = R"ROC(
{ domain: "[I,J] -> { B[i, j] : 0 < i <= I and 0 < j <= J; A[i, j] : 0 < i <= I and 0 < j <= J }",
  child:
   { sequence:
      [ { filter: "{ A[i, j] }",
          child:
           { schedule: "[{ A[i, j] -> [ 4 * floor(i/4)] }, { A[i, j] -> [i] }, { A[i,j] -> [4*floor(j/4)] }, { A[i,j] -> [j] } ]",
             permutable: 1,
             coincident: [ 1, 1 ],

             child:
               { schedule: "[{ A[i, j] -> [i+j] }, { A[i, j] -> [i] } ]",
                 permutable: 1,
                 coincident: [ 1, 1 ],

                 child:
                   { schedule: "[{ A[i, j] -> [j] }, { A[i, j] -> [i] } ]",
                     permutable: 1,
                     coincident: [ 1, 1 ]
                   }
           }
        }
    },
        { filter: "{ B[i, j] }",
          child:
           { schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]",
             permutable: 1,
             coincident: [ 1, 1 ] } } ] } }
  )ROC";

  LOG(INFO) << schedule_repr;

  isl::ctx ctx(isl_ctx_alloc());
  isl::schedule schedule = isl::manage(isl_schedule_read_from_str(ctx.get(), schedule_repr.c_str()));

  DisplayScheduleC(schedule.get());
}

TEST(isl, schedule_tree_tile) {
  std::string schedule_repr = R"ROC(
{ domain: "[I,J] -> { A[i, j] : 0 < i <= I and 0 < j <= J }",
  child:
   { sequence:
      [ { filter: "{ A[i, j] }",
          child:
              { schedule: "[{ A[i, j] -> [i+j] }, { A[i,j] -> [j] } ]",
                permutable: 1,
                coincident: [ 1, 1 ],
          child:
              { schedule: "[{ A[i, j] -> [j] }, { A[i,j] -> [i] } ]",
                permutable: 1,
                coincident: [ 1, 1 ]}
}
 }
] } }
  )ROC";

  isl::ctx ctx(isl_ctx_alloc());
  isl::schedule schedule = isl::manage(isl_schedule_read_from_str(ctx.get(), schedule_repr.c_str()));
  LOG(INFO) << "schedule: " << schedule.get_root();

  DisplayScheduleC(schedule.get());
}

TEST(isl, align) {
  isl_ctx *ctx = isl_ctx_alloc();

  isl::map schedule0(ctx, "{ S0[i,j] -> [0, i, 0, j] }");
  isl::map schedule1(ctx, "{ S1[i,j,k] -> [0, i, 0, j, 0, k] }");
  isl::map schedule2(ctx, "{ S2[k] -> [0, k] }");

  int max_range_dim = 6;

  LOG(INFO) << "map.domain: " << schedule0.domain();
  LOG(INFO) << "map.domain.space: " << schedule0.domain().space();
  LOG(INFO) << "map.range: " << schedule0.range();
  LOG(INFO) << "map.range.space: " << schedule0.range().space();

  int mdim = isl_map_dim(schedule0.get(), isl_dim_out);
  schedule0 = isl::manage(isl_map_add_dims(schedule0.release(), isl_dim_out, max_range_dim - mdim));
  LOG(INFO) << "after add dim: " << schedule0;

  for (int i = mdim; i < max_range_dim; i++) {
    isl::space sp = schedule0.space();
    isl_local_space *lsp = isl_local_space_from_space(sp.copy());
    isl_constraint *cst = isl_constraint_alloc_equality(lsp);
    cst = isl_constraint_set_coefficient_si(cst, isl_dim_out, i, 1);
    schedule0 = isl::manage(isl_map_add_constraint(schedule0.release(), cst));
  }

  LOG(INFO) << "after process: " << schedule0;
}

TEST(isl, identity_schedule) {
  isl_ctx *ctx = isl_ctx_alloc();
  isl::set domain(ctx, "[n] -> { A[i,j] : 0 < i < j < n }");
  isl_space *domain_space = isl_space_copy(domain.space().get());
  isl::map scheduled = isl::manage(isl_map_identity(isl_space_map_from_set(domain_space)));
  scheduled = isl::manage(isl_map_intersect_domain(scheduled.release(), domain.copy()));
  LOG(INFO) << "scheduled: " << scheduled;
  scheduled = isl::manage(isl_map_set_tuple_name(scheduled.release(), isl_dim_out, ""));
  scheduled = isl::manage(isl_map_coalesce(scheduled.release()));
  LOG(INFO) << "scheduled: " << scheduled;
}

// TODO How to use the flow?
TEST(isl, dependency_analysis) {
  /*
   * for (int i = 0; i < N; i++) {
   *   A[i] = 0; //         S0(i)
   * }
   *
   * for (int i = 0; i < N; i++) {
   *   for (int j = 0; j < N; j++) {
   *     A[i] += B[i,j];  // S1(i,j)
   *   }
   * }
   */
  isl_ctx *ctx = isl_ctx_alloc();
  isl::union_map reads(ctx, "[N,M] -> { S1[i,j]->A[i]; S1[i,j]->B[i,j] }");
  isl::union_map writes(ctx, "[N,M]-> { S0[i]->A[i]; S1[i,j]->A[i] }");

  isl_union_access_info *ai;
  ai = isl_union_access_info_from_sink(reads.copy());
  ai = isl_union_access_info_set_must_source(ai, writes.copy());
  LOG(INFO) << "ai: " << isl_union_access_info_to_str(ai);
  auto *flow = isl_union_access_info_compute_flow(ai);
  LOG(INFO) << "flow: " << isl_union_flow_to_str(flow);
  isl::union_map raw = isl::manage(isl_union_flow_get_must_dependence(flow));
  LOG(INFO) << "raw: " << raw;
}

TEST(isl, range) {
  auto *ctx = isl_ctx_alloc();
  isl::map access(ctx, "[N] -> { S[i,j] -> A[i+1, j-1] : 0 < 2*i+1,j < N }");
  isl::map map(ctx, "[N] -> { A[i,j] -> [i] }");

  LOG(INFO) << "map applied: " << access.apply_range(map);

  LOG(INFO) << "access: " << access;
  auto box = access.get_range_simple_fixed_box_hull();
  LOG(INFO) << "size: " << box.get_offset().get_at(0);
  LOG(INFO) << "box: " << box;
}

TEST(isl, concat_cond) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::set set(ctx.get(), "{ A[i,j] : 0 < i < j < 100 }");
  set = isl::manage(cinn::isl_set_append_cond(set.release(), "i % 2 = 0"));
}
