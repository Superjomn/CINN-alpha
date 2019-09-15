#include "cinn/core/stage.h"
#include <gtest/gtest.h>
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"
#include "cinn/utils/string.h"

namespace cinn {
using namespace ir;

TEST(Stage, from_expr) {
  Expr A("A"), B("B"), C("C");
  ir::Var i("i", primitive_t::int64, Constant(0), Constant(100));
  ir::Var j("j", primitive_t::int64, Constant(0), Constant(100));

  Expr A_a = A[i][j].Assign(B[i][j] + C[i][j]);

  LOG(INFO) << "A_a: " << Dump(A_a);

  Stage s0 = A_a;
  LOG(INFO) << "s0 info " << s0.DumpIslC();
}

TEST(Stage, multiple) {
  Expr A("A"), B("B"), C("C");
  Var i("i", 0, 100);
  Var j("j", 0, 200);
  Var k("k", 0, 300);

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  LOG(INFO) << "s0: " << s0.DumpIslC();
  LOG(INFO) << "s1: " << s1.DumpIslC();
}

TEST(Stage, DumpC) {
  Expr A("A"), B("B"), C("C");
  Var i("i", 0, 100);
  Var j("j", 0, 200);
  Var k("k", 0, 300);

  Stage s0 = C[i][j].Assign(A[i][k] * B[k][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  LOG(INFO) << "s0: " << s0.DumpAsC();
}

TEST(Stage, Interchange) {
  Expr A("A"), B("B"), C("C");
  Var i("i", 0, 100);
  Var j("j", 0, 200);
  Var k("k", 0, 300);

  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    s0.Interchange(0, 1);

    auto repr = GetStreamStr(s0.schedule());
    ASSERT_EQ(
        repr,
        StringFormat("{ %s[i, j, k] -> %s[j' = j, i' = i, k' = k] : 0 <= i <= 99 and 0 <= j <= 199 and 0 <= k <= 299 }",
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
        StringFormat("{ %s[i, j, k] -> %s[j' = j, i' = i, k' = k] : 0 <= i <= 99 and 0 <= j <= 199 and 0 <= k <= 299 }",
                     s0.name().c_str(),
                     s0.name().c_str()));
    LOG(INFO) << "schedule: " << s0.schedule();
    LOG(INFO) << s0.DumpIslC();
    ASSERT_EQ(s0.DumpIslC(),
              StringFormat("for (int j = 0; j <= 199; j += 1)\n  for (int i = 0; i <= 99; i += 1)\n    for (int k = 0; "
                           "k <= 299; k += 1)\n      %s(i, j, k);\n",
                           s0.name().c_str()));
  }
}

TEST(Stage, InitRWAccess) {
  Expr A("A"), B("B"), C("C");
  Var i("i", 0, 100);
  Var j("j", 0, 200);
  Var k("k", 0, 300);

  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    auto repr = GetStreamStr(s0.schedule());
    LOG(INFO) << "schedule: " << s0.schedule();
    LOG(INFO) << "read access: " << isl_union_map_to_str(s0.read_access());
    LOG(INFO) << "write access: " << isl_union_map_to_str(s0.write_access());
    ASSERT_EQ(isl_union_map_to_str(s0.read_access()),
              StringFormat(
                  "{ %s[i, j, k] -> A[1 + i, k]; %s[i, j, k] -> B[-1 + k, j] }", s0.name().c_str(), s0.name().c_str()));
    ASSERT_EQ(isl_union_map_to_str(s0.write_access()), StringFormat("{ %s[i, j, k] -> C[i, j] }", s0.name().c_str()));
  }
}

}  // namespace cinn