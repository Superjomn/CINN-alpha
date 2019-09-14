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
    ASSERT_EQ(repr,
              "{ " + s0.name() + "[i, j] -> " + s0.name() + "[j' = j, i' = i] : 0 <= i <= 99 and 0 <= j <= 199 }");
    LOG(INFO) << "schedule: " << s0.schedule();
  }
  {
    Stage s0 = C[i][j].Assign(A[i + 1][k] * B[k - 1][j]);
    s0.Interchange(i, j);

    auto repr = GetStreamStr(s0.schedule());
    ASSERT_EQ(repr,
              "{ " + s0.name() + "[i, j] -> " + s0.name() + "[j' = j, i' = i] : 0 <= i <= 99 and 0 <= j <= 199 }");
    LOG(INFO) << "schedule: " << s0.schedule();
    LOG(INFO) << s0.DumpIslC();
    ASSERT_EQ(s0.DumpIslC(),
              StringFormat(R"DOC(for (int j = 0; j <= 199; j += 1)
  for (int i = 0; i <= 99; i += 1)
    %s(i, j);
)DOC",
                           s0.name().c_str()));
  }
}

}  // namespace cinn