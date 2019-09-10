#include "cinn/core/stage.h"
#include <gtest/gtest.h>
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
using namespace ir;

TEST(Stage, from_expr) {
  Expr A("A"), B("B"), C("C");
  ir::Var i("i", primitive_t::int64, Parameter(0), Parameter(100));
  ir::Var j("j", primitive_t::int64, Parameter(0), Parameter(100));

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

}  // namespace cinn