#include "cinn/core/function.h"
#include <gtest/gtest.h>
#include "cinn/core/stage.h"
#include "cinn/ir/ir_printer.h"
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

  LOG(INFO) << "func0 " << ir::Dump(Expr(func0));
}

TEST(Function, GenerateIslAst) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);

  Expr A("A"), B("B"), C("C");

  Stage s0 = C[i][j].Assign(A[i][j] * B[i][j]);
  Stage s1 = C[i][j].Assign(C[i][j] + 1);

  auto func0 = Function::make("func0", {B, C}, {A}, {s0, s1});
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

  // auto final_transform = func0->GetFinalTransform();
  // LOG(INFO) << "final_transform: " << final_transform;
  // LOG(INFO) << "dump: \n" << func0->DumpIslC();
}

TEST(Function, Dump) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);
  Var k("k", 0, 1500);

  Expr A("A"), B("B"), C("C"), D("D");

  Stage s0 = C[i][j].Assign(A[i][j * 2] * B[i - 3][j]);
  Stage s1 = C[i][j].Assign((C[i][j] + C[i + 1][j + 1] + C[i - 1][j - 1]) / 3);
  Stage s2 = B[k].Assign(Expr(1));

  auto func0 = Function::make("func0", {A, B, C}, {B, C}, {s0, s1, s2});

  LOG(INFO) << "Dump: \n" << ir::Dump(Expr(func0));
}

TEST(Function, buffer_allocate) {
  Var i("i", 0, 100);
  Var j("j", 0, 150);
  Var k("k", 0, 1500);

  Expr A("A"), B("B"), C("C"), D("D");

  Stage s0 = C[i][j].Assign(A[i][j * 2] * B[i - 3][j]);
  Stage s_alloc_D = Allocate::make("D", Expr(10), primitive_t::float32);

  auto func0 = Function::make("func0", {A, B}, {C}, {s_alloc_D, s0});

  LOG(INFO) << "func0.code : \n" << ir::Dump(Expr(func0));
}

Expr tanh(Expr x) { return Max::make(Expr(0.f), x); }

TEST(Snippet, test) {
  Snippet snippet0;

  int N = 1000;
  int M = 200;

  Expr x("x");
  Expr y("y");

  Var i("i", primitive_t::int32, 0, N);
  Var j("j", primitive_t::int32, 0, M);

  Stage s0 = x[i][j].Assign(Expr(1));
  Stage s1 = y[i][j].Assign(x[i][j] + 1);

  snippet0.AddStage(s0);
  snippet0.AddStage(s1);
  snippet0.End();
}

TEST(Snippet, test1) {
  std::vector<Snippet> snippets;
  for (int i = 0; i < 10; i++) {
    snippets.emplace_back();
  }
}

TEST(Function, syntax) {
  // Param N("N", "N>10 and N < 10000");
  // Param M("M", "M > 10");
  int N = 1000;
  int M = 200;
  // LOG(INFO) << N.context();
  // LOG(INFO) << M.context();

  Function tanh_fn("tanh");
  {
    Expr x("x");
    Expr y("y");

    Var i("i", primitive_t::int32, 0, N);
    Var j("j", primitive_t::int32, 0, M);

    tanh_fn.Inputs({x});
    tanh_fn.Outputs({y});
    auto s0 = tanh_fn.AddStage(y[i][j].Assign(tanh(x[i][j])));
    LOG(INFO) << "s0.expr: " << ir::Dump(s0.expr());
  }
  tanh_fn.EndDefinition();

  Buffer input("input", {Expr(N), Expr(M)}, primitive_t::float32);
  Buffer output("output", {Expr(N), Expr(M)}, primitive_t::float32);

  Function main("main");
  {
    Expr x("x"), y("y");

    main.AddStage(tanh_fn({x}, {y}));
    main.Inputs({x});
    main.Outputs({y});

    x.Bind(input);
    y.Bind(output);
  }
  main.EndDefinition();

  LOG(INFO) << "function tanh_h: \n" << ir::Dump(tanh_fn);

  LOG(INFO) << "function main: \n" << ir::Dump(main);

  // LOG(INFO) << "function tanh: \n" << ir::Dump(tanh_fn);
}

}  // namespace cinn
