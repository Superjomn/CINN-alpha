#include "cinn/core/stage.h"
#include <gtest/gtest.h>
#include <vector>
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/string.h"

namespace cinn {
using namespace ir;

using cs = std::vector<Constant>;

TEST(Stage, from_expr) {
  Constant M(20);
  Constant K(10);
  Constant N(30);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Var i, j;

  Stage s0 = A[i][j].Assign(B[i][j] + C[i][j]);

  LOG(INFO) << "s0 info " << s0.DumpIslC();
}

TEST(Stage, multiple) {
  Constant M(20);
  Constant K(10);
  Constant N(30);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Var i, j, k;

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1.f);

  LOG(INFO) << "s0: " << s0.DumpIslC();
  LOG(INFO) << "s1: " << s1.DumpIslC();
}

TEST(Stage, DumpC) {
  Constant M(20);
  Constant K(10);
  Constant N(30);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Var i, j, k;

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1.f);

  LOG(INFO) << "s0: " << s0.DumpAsC();
}

TEST(Stage, InitRWAccess) {
  Constant M(20);
  Constant K(10);
  Constant N(30);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Var i("i"), j("j"), k("k");

  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    auto repr = GetStreamStr(s0.schedule());
    LOG(INFO) << "schedule: " << s0.schedule();
    LOG(INFO) << "read access: " << isl_union_map_to_str(s0.read_access());
    LOG(INFO) << "write access: " << isl_union_map_to_str(s0.write_access());

    isl::union_map read_target(
        s0.schedule().ctx(),
        StringFormat(
            "{ %s[i, j, k] -> B[-1 + k, j]; %s[i, j, k] -> A[1 + i, k] }", s0.name().c_str(), s0.name().c_str()));
    isl::union_map write_target(s0.schedule().ctx(), StringFormat("{ %s[i, j, k] -> C[i, j] }", s0.name().c_str()));

    ASSERT_TRUE(isl_union_map_is_equal(s0.read_access(), read_target.get()));
    ASSERT_TRUE(isl_union_map_is_equal(s0.write_access(), write_target.get()));
  }
}

TEST(Stage, BuildDomainFromDimensions) {
  Constant I("I", 30);
  Constant J("J", 40);
  Constant M("M", 20);

  auto domains = BuildDomainFromDimensions({I, J, M}, {"ii0", "ii1", "ii2"});
  LOG(INFO) << "domains: " << domains;
  ASSERT_EQ(GetStreamStr(domains), "{ [ii0, ii1, ii2] : 0 <= ii0 <= 29 and 0 <= ii1 <= 39 and 0 <= ii2 <= 19 }");
}

TEST(Stage, BuildDomainFromExprWithDimension) {
  Constant I("I", 30);
  Constant J("J", 40);
  Constant M("M", 20);

  Var i("i0"), j("i1"), k("i2");

  LOG(INFO) << "i: " << i.name();
  LOG(INFO) << "j: " << j.name();
  LOG(INFO) << "k: " << k.name();
  std::vector<Expr> iterators({Expr(i) * 2, Expr(j) + 3, Expr(k) - 2});

  auto domain = BuildDomainFromExprWithDimension(iterators, {I, J, M});
  LOG(INFO) << "domain " << domain;
  ASSERT_EQ(GetStreamStr(domain), "{ [i0, i1, i2] : 0 <= i0 <= 14 and -3 <= i1 <= 36 and 2 <= i2 <= 21 }");
}

TEST(Stage, syntax) {
  Constant I("I", 30);
  Constant J("J", 40);
  Constant M("M", 20);

  Var i, j, k;

  Expr A(std::vector<Constant>({I, M}), primitive_t::float32, "A");
  Expr B(std::vector<Constant>({M, J}), primitive_t::float32, "B");
  Expr C(std::vector<Constant>({I, J}), primitive_t::float32, "C");

  Stage s0 = C[i][j].Assign(A[i * 2][k + 3] * B[k + 3][j]);
  LOG(INFO) << "s0.iterator_domain: " << s0.iterator_domain();
  Stage s1 = C[i][j].Assign(C[i][j] + 1.f);
  LOG(INFO) << "s1.iterator_domain: " << s1.iterator_domain();
  LOG(INFO) << "s0.code: \n" << s0.DumpIslC();
  LOG(INFO) << "s1.code: \n" << s1.DumpIslC();
}

TEST(Stage, Split) {
  Constant I("I", 30);
  Constant J("J", 40);
  Constant M("M", 20);

  Var i, j, k;

  Expr A(std::vector<Constant>({I, M}), primitive_t::float32, "A");
  Expr B(std::vector<Constant>({M, J}), primitive_t::float32, "B");
  Expr C(std::vector<Constant>({I, J}), primitive_t::float32, "C");

  Stage s0 = C[i][j].Assign(A[i * 2][k + 3] * B[k + 3][j]);
  s0.Split(i, 4);

  LOG(INFO) << "After split: \n" << s0.DumpIslC();
  LOG(INFO) << "After split: \n" << s0.DumpAsC();
}

TEST(Stage, Tile) {
  Constant I("I", 100);
  Constant J("J", 200);
  Constant M("M", 300);

  Var i, j, k;

  Expr A(std::vector<Constant>({I, M}), primitive_t::float32, "A");
  Expr B(std::vector<Constant>({M, J}), primitive_t::float32, "B");
  Expr C(std::vector<Constant>({I, J}), primitive_t::float32, "C");

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  s0.Tile(i, 4);

  LOG(INFO) << "After split: \n" << s0.DumpIslC();
  LOG(INFO) << "After split: \n" << s0.DumpAsC();
}

TEST(Stage, cond) {
  Constant I("I", 100);
  Constant J("J", 200);
  Constant M("M", 300);

  Var i, j, k;

  Expr A(std::vector<Constant>({I, M}), primitive_t::float32, "A");
  Expr B(std::vector<Constant>({M, J}), primitive_t::float32, "B");
  Expr C(std::vector<Constant>({I, J}), primitive_t::float32, "C");

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  s0.SetCond(i, "% 2 = 0");

  auto log = s0.DumpIslC();
  LOG(INFO) << "code gen: \n" << log;

  auto target = StringFormat(R"ROC(for (int %s = 0; %s <= 99; %s += 2)
  for (int %s = 0; %s <= 199; %s += 1)
    for (int %s = 0; %s <= 299; %s += 1)
      %s(%s, %s, %s);
)ROC",
                             i.name().c_str(),
                             i.name().c_str(),
                             i.name().c_str(),
                             j.name().c_str(),
                             j.name().c_str(),
                             j.name().c_str(),
                             k.name().c_str(),
                             k.name().c_str(),
                             k.name().c_str(),
                             s0.name().c_str(),
                             i.name().c_str(),
                             j.name().c_str(),
                             k.name().c_str());

  EXPECT_EQ(log, target);
}

}  // namespace cinn
