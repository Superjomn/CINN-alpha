#include "cinn/core/stage.h"
#include <gtest/gtest.h>
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
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

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
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  LOG(INFO) << "s0: " << s0.DumpAsC();
}

TEST(Stage, Interchange) {
  Constant M(100);
  Constant K(200);
  Constant N(300);
  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Var i("i"), j("j"), k("k");

  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    s0.Interchange(0, 1);

    auto repr = GetStreamStr(s0.schedule());
    ASSERT_EQ(
        repr,
        StringFormat("{ %s[i, j, k] -> %s[j' = j, i' = i, k' = k] : 0 <= i <= 99 and 0 <= j <= 300 and 0 < k <= 200 }",
                     s0.name().c_str(),
                     s0.name().c_str()));
    LOG(INFO) << "schedule: " << s0.schedule();
  }
  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    s0.Interchange(i, j);

    auto repr = GetStreamStr(s0.schedule());
    ASSERT_EQ(
        repr,
        StringFormat("{ %s[i, j, k] -> %s[j' = j, i' = i, k' = k] : 0 <= i <= 99 and 0 <= j <= 300 and 0 < k <= 200 }",
                     s0.name().c_str(),
                     s0.name().c_str()));
    LOG(INFO) << "schedule: " << s0.schedule();
    LOG(INFO) << s0.DumpIslC();
    ASSERT_EQ(s0.DumpIslC(),
              StringFormat("for (int j = 0; j <= 300; j += 1)\n  for (int i = 0; i <= 99; i += 1)\n    for (int k = 1; "
                           "k <= 200; k += 1)\n      %s(i, j, k);\n",
                           s0.name().c_str()));
  }
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
    ASSERT_EQ(isl_union_map_to_str(s0.read_access()),
              StringFormat(
                  "{ %s[i, j, k] -> B[-1 + k, j]; %s[i, j, k] -> A[1 + i, k] }", s0.name().c_str(), s0.name().c_str()));
    ASSERT_EQ(isl_union_map_to_str(s0.write_access()), StringFormat("{ %s[i, j, k] -> C[i, j] }", s0.name().c_str()));
  }
}

TEST(Stage, BuildDomainFromDimensions) {
  Constant I("I", 30);
  Constant J("J", 40);
  Constant M("M", 20);

  auto domains = BuildDomainFromDimensions({I, J, M}, {"ii0", "ii1", "ii2"});
  LOG(INFO) << "domains: " << domains;
  ASSERT_EQ(GetStreamStr(domains), "{ [ii0, ii1, ii2] : 0 <= ii0 <= 30 and 0 <= ii1 <= 40 and 0 <= ii2 <= 20 }");
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
  ASSERT_EQ(GetStreamStr(domain), "{ [i0, i1, i2] : 0 <= i0 <= 15 and -3 <= i1 <= 37 and 2 <= i2 <= 22 }");
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
  Stage s1 = C[i][j].Assign(C[i][j] + 1);
  LOG(INFO) << "s1.iterator_domain: " << s1.iterator_domain();
  LOG(INFO) << "s0.code: \n" << s0.DumpIslC();
  LOG(INFO) << "s1.code: \n" << s1.DumpIslC();
}

}  // namespace cinn