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
    if (isl_val_is_one(val))  // i.e., the coefficient of the dimension data->in_dim is 1
    {
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
