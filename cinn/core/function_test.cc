#include "cinn/core/function.h"
#include <gtest/gtest.h>
#include "cinn/core/stage.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
using namespace ir;

TEST(Function, basic) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);

  Expr A("A"), B("B"), C("C");

  Stage s0 = C[i][j].Assign(A[i][j] * B[i][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  auto func0 = Function::make("func0", {B, C}, {A}, {s0, s1});

  LOG(INFO) << "func0 " << func0->DumpIslC();
}

TEST(Function, GenerateIslAst) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);

  Expr A("A"), B("B"), C("C");

  Stage s0 = C[i][j].Assign(A[i][j] * B[i][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  auto func0 = Function::make("func0", {B, C}, {A}, {s0, s1});
  LOG(INFO) << "iter2 " << func0->iterator_domain();
}

TEST(Function, DumpIslC) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);
  Var k("k", 0, 1500);

  Expr A("A"), B("B"), C("C"), D("D");

  Stage s0 = C[i][j].Assign(A[i][j] * B[i][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);
  Stage s2 = B[k].Assign(Expr(1));

  auto func0 = Function::make("func0", {A, B, C}, {B, C}, {s0, s1, s2});

  auto final_transform = func0->GetFinalTransform();
  LOG(INFO) << "final_transform: " << final_transform;
  LOG(INFO) << "dump: \n" << func0->DumpIslC();
}

TEST(Function, Dump) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);
  Var k("k", 0, 1500);

  Expr A("A"), B("B"), C("C"), D("D");

  Stage s0 = C[i][j].Assign(A[i][j] * B[i][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);
  Stage s2 = B[k].Assign(Expr(1));

  auto func0 = Function::make("func0", {A, B, C}, {B, C}, {s0, s1, s2});

  LOG(INFO) << "ISL C dump:\n" << func0->DumpIslC();
  LOG(INFO) << "Dump: \n" << func0->Dump();
}

}  // namespace cinn