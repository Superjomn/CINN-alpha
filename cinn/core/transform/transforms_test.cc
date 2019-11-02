#include "cinn/core/transform/transforms.h"
#include <gtest/gtest.h>
#include "cinn/utils/isl_utils.h"

namespace cinn {

TEST(tile, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "schedule: " << schedule;

  TileSingleDimTransformer appler("A", "i", 32);
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

  TileUnrollTransformer appler("A", {32, 32});
  schedule = appler.Visit(schedule).get_schedule();
  LOG(INFO) << "schedule: \n" << schedule;
  LOG(INFO) << "final schedule: \n" << schedule.root();

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  LOG(INFO) << "code: \n" << isl_ast_node_to_C_str(ast);
}

TEST(transpose, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 10; C[i,j,k]: 0 < i,j,k < 10; B[i,j] : 0<i,j<10 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");
  isl::union_map proximity(ctx, "{ A[i,j] -> C[i,j,k] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  // sc = sc.set_proximity(proximity);
  isl::schedule schedule = sc.compute_schedule();
  LOG(INFO) << "original schedule\n" << schedule.get_root();

  InterchangeTransformer appler("C", "i", "k");
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

TEST(tile_dims, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j] : 0 < i,j < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();

  TileDimsTransformer appler("A", {32, 32});
  schedule = appler.Visit(schedule).get_schedule();
  LOG(INFO) << "Final schedule: \n" << schedule << std::endl;
  std::string target =
      R"ROC({ domain: "{ B[i, j] : 0 < i <= 99 and 0 < j <= 99; A[i, j] : 0 < i <= 199 and 0 < j <= 199 }",
  child:
   { sequence:
      [ { filter: "{ A[i, j] }",
          child:
           { mark: "tile - tiles",
             child:
              { schedule: "[{ A[i, j] -> [(32*floor((i)/32))] }, { A[i, j] -> [(32*floor((j)/32))] }]",
                permutable: 1,
                coincident: [ 1, 1 ],
                child:
                 { mark: "tile - points",
                   child:
                    { schedule: "[{ A[i, j] -> [(i - 32*floor((i)/32))] }, { A[i, j] -> [(j - 32*floor((j)/32))] }]",
                      permutable: 1,
                      coincident: [ 1, 1 ] } } } } },
        { filter: "{ B[i, j] }",
          child:
           { schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]",
             permutable: 1,
             coincident: [ 1, 1 ] } } ] } }
  )ROC";

  auto *traget_schedule = isl_schedule_read_from_str(schedule.ctx().get(), target.c_str());
  // ASSERT_TRUE(isl_schedule_plain_is_equal(schedule.get(), traget_schedule));

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  LOG(INFO) << std::endl;
  LOG(INFO) << "code: " << std::endl << isl_ast_node_to_C_str(ast) << std::endl;
}

TEST(tile_dims, tile_unroll) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::union_set domain(ctx, "{ A[i,j,k] : 0 < i,j,k < 200; B[i,j] : 0<i,j<100 }");
  isl::union_map validity(ctx, "{ A[i,j,k] -> B[i,j] }");

  isl::schedule_constraints sc = isl::schedule_constraints::on_domain(domain);
  sc = sc.set_validity(validity);
  isl::schedule schedule = sc.compute_schedule();

  TileDimsTransformer appler("A", {32, 32}, true /*unroll*/);
  schedule = appler.Visit(schedule).get_schedule();
  LOG(INFO) << "Final schedule: \n" << schedule << std::endl;
  std::string target =
      R"ROC({ domain: "{ B[i, j] : 0 < i <= 99 and 0 < j <= 99; A[i, j] : 0 < i <= 199 and 0 < j <= 199 }",
  child:
   { sequence:
      [ { filter: "{ A[i, j] }",
          child:
           { schedule: "[{ A[i, j] -> [(32*floor((i)/32))] }, { A[i, j] -> [(32*floor((j)/32))] }]",
             permutable: 1,
             coincident: [ 1, 1 ],
             child:
              { schedule: "[{ A[i, j] -> [(i - 32*floor((i)/32))] }, { A[i, j] -> [(j - 32*floor((j)/32))] }]",
                permutable: 1,
                coincident: [ 1, 1 ] } } },
        { filter: "{ B[i, j] }",
          child:
           { schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]",
             permutable: 1,
             coincident: [ 1, 1 ] } } ] } }
  )ROC";

  auto *traget_schedule = isl_schedule_read_from_str(schedule.ctx().get(), target.c_str());
  // ASSERT_TRUE(isl_schedule_plain_is_equal(schedule.get(), traget_schedule));

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule.copy());
  LOG(INFO) << std::endl;
  LOG(INFO) << "code: " << std::endl << isl_ast_node_to_C_str(ast) << std::endl;
}

TEST(isl, split) {
  isl::ctx ctx(isl_ctx_alloc());
  std::string target =
      R"ROC(
{ domain: "{ B[i, j] : 0 < i <= 99 and 0 < j <= 99; A[i, j, k] : 0 < i <= 199 and 0 < j <= 199 and 0 < k <= 199 }",
  child:
   { sequence:
      [ { filter: "{ A[i, j, k] }",
          child:
           { mark: "tile-unroll - tiles",
             child:
              { schedule: "[{ A[i, j, k] -> [(i)] }, { A[i, j, k] -> [(32*floor((j)/32))] }, { A[i, j, k] -> [(32*floor((k)/32))] }]",
                permutable: 1,
                coincident: [ 1, 1, 1 ],
                child:
                 { mark: "tile-unroll - points",
                   child:
                    { schedule: "[{ A[i, j, k] -> [(0)] }, { A[i, j, k] -> [(j - 32*floor((j)/32))] }, { A[i, j, k] -> [(k - 32*floor((k)/32))] }]",
                      permutable: 1,
                      coincident: [ 1, 1, 1 ],
                      options: "{ [isolate[] -> unroll[3]]; isolate[[]->[i0, i1, i2, i4,i5]] : (i1) mod 32 = 0 and (i2) mod 32 = 0 and 0 < i0 <= 199 and 0 <= i4 <= 31 and 0 <= i5 <= 31 and 32*floor((-i2 + i5)/32) >= -199 + i5 and 32*floor((-i2 + i5)/32) < i5 and 32*floor((-i1 + i4)/32) >= -199 + i4 and 32*floor((-i1 + i4)/32) < i4 }" } } } } },
        { filter: "{ B[i, j] }",
          child:
           { schedule: "[{ B[i, j] -> [(i)] }, { B[i, j] -> [(j)] }]",
             permutable: 1,
             coincident: [ 1, 1 ] } } ] } }

  )ROC";

  auto *schedule = isl_schedule_read_from_str(ctx.get(), target.c_str());

  auto *build = isl_ast_build_from_context(isl_set_read_from_str(ctx.get(), "{:}"));
  auto *ast = isl_ast_build_node_from_schedule(build, schedule);
  LOG(INFO) << "code: " << std::endl << isl_ast_node_to_C_str(ast);
}

TEST(GetPartialTilePrefixes, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::set set(ctx, "{ S[i,j, k] : 0<= i,j,k <200 }");
  set = GetPartialTilePrefixes(set, 8);
  LOG(INFO) << "set: " << set;
}

TEST(GetIsolateOptions, basic) {
  isl::ctx ctx(isl_ctx_alloc());
  isl::set set(ctx, "{ S[i,j,k] : 0<= i,j,k <200 }");
  auto out = GetIsolateOptions(set, 2);
  LOG(INFO) << "out " << out;
}

}  // namespace cinn
