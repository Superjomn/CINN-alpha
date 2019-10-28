#include "cinn/core/transform/transforms.h"
#include <gtest/gtest.h>

namespace cinn {

TEST(tile, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "schedule: " << schedule;

  TileTransformer appler("A", "i", 32);
  std::string target_schedule =
      "{ domain: \"{ B[i, j] : 0 < i <= 99 and 0 < j <= 99; A[i, j] : 0 < i <= 199 and 0 < j <= 199 }\", child: { "
      "sequence: [ { filter: \"{ A[i, j] }\", child: { schedule: \"[{ A[i, j] -> [(-32 - 32*floor((-1 - i)/32))] }, { "
      "A[i, j] -> [(i)] }, { A[i, j] -> [(j)] }]\", child: { schedule: \"[{ A[i, j] -> [(i)] }, { A[i, j] -> [(j)] "
      "}]\", permutable: 1, coincident: [ 1, 1 ] } } }, { filter: \"{ B[i, j] }\", child: { schedule: \"[{ B[i, j] -> "
      "[(i)] }, { B[i, j] -> [(j)] }]\", permutable: 1, coincident: [ 1, 1 ] } } ] } }";
  schedule = appler.Visit(schedule).get_schedule();

  LOG(INFO) << "schedule: " << schedule;
  ASSERT_EQ(GetStreamStr(schedule), target_schedule);
}

TEST(tile, tile_from_tail) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();

  TileTransformer2 appler("A", {32, 32});
  schedule = appler.Visit(schedule).get_schedule();
  LOG(INFO) << "schedule: \n" << schedule;
  LOG(INFO) << "final schedule: \n" << schedule.root();

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  LOG(INFO) << "code: \n" << isl_ast_node_to_C_str(ast);
}

TEST(unroll, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 10; B[i,j] : 0<i,j<10 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();

  UnrollTransformer appler("A", "i");
  schedule = appler.Visit(schedule).get_schedule();
  LOG(INFO) << "schedule \n" << schedule;
  LOG(INFO) << schedule.root();

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  LOG(INFO) << "code: \n" << isl_ast_node_to_C_str(ast);
}

TEST(skew, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 10; C[i,j,k]: 0 < i,j,k < 10; B[i,j] : 0<i,j<10 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");
  isl::union_map proximity(ctx, "{ A[i,j] -> C[i,j,k] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  // sc = sc.set_proximity(proximity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "original schedule\n" << schedule.get_root();

  TransposeTransformer appler("C", "i", "k");
  schedule = appler.Visit(schedule).get_schedule();

  LOG(INFO) << "final schedule\n" << schedule.get_root();
  auto target = R"ROC(# YOU ARE HERE
domain: "{ C[i, j, k] : 0 < i <= 9 and 0 < j <= 9 and 0 < k <= 9; B[i, j] : 0 < i <= 9 and 0 < j <= 9; A[i, j] : 0 < i <= 9 and 0 < j <= 9 }"
child:
  set:
  - filter: "{ B[i, j]; A[i, j] }"
    child:
      sequence:
      - filter: "{ A[i, j] }"
        child:
          schedule: "[{ A[i, j] -> [(i)] }, { A[i, j] -> [(j)] }]"
          permutable: 1
          coincident: [ 1, 1 ]
      - filter: "{ B[i, j] }"
        child:
          schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]"
          permutable: 1
          coincident: [ 1, 1 ]
  - filter: "{ C[i, j, k] }"
    child:
      schedule: "[{ C[i, j, k] -> [(k)] }, { C[i, j, k] -> [(j)] }, { C[i, j, k] -> [(i)] }]"
      child:
        schedule: "[{ C[i, j, k] -> [(i)] }, { C[i, j, k] -> [(j)] }, { C[i, j, k] -> [(k)] }]"
        permutable: 1
        coincident: [ 1, 1, 1 ]
)ROC";
  auto gen = GetStreamStr(schedule.get_root());
  ASSERT_EQ(gen, target);
}

}  // namespace cinn