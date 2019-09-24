#include "cinn/core/function.h"
#include <gtest/gtest.h>
#include "cinn/core/stage.h"
#include "cinn/ir/ir_helper.h"
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
  using cs = std::vector<Constant>;

  Constant N(1000);
  Constant M(1000);

  Function tanh_fn("tanh");
  {
    Expr X(std::vector<Constant>({N}), primitive_t::float32);
    Expr Out(std::vector<Constant>({N}), primitive_t::float32);

    Var i;

    tanh_fn.Inputs({X});
    tanh_fn.Outputs({Out});

    auto s0 = tanh_fn.AddStage(Out[i].Assign(ir::tanh(X[i])));

    tanh_fn.EndDefinition();
  }

  LOG(INFO) << "tanh:\n" << ir::Dump(tanh_fn);

  Function softmax_fn("softmax");
  {
    Constant N(10);
    Constant D(20);
    Expr I(cs({N, D}), primitive_t::float32);
    Expr expsum(cs({N}), primitive_t::float32);
    Expr O(cs({N, D}), primitive_t::float32);

    softmax_fn.Inputs({I});
    softmax_fn.Outputs({expsum, O});

    Var n, d;

    auto s0 = softmax_fn.AddStage(  //
        expsum[n].Assign(expsum[n] + ir::exp(I[n][d])));
    auto s1 = softmax_fn.AddStage(  //
        O[n][d].Assign(exp(I[n][d]) / expsum[n]));

    softmax_fn.EndDefinition();
  }

  LOG(INFO) << "softmax: \n" << ir::Dump(softmax_fn);

  Function matmul_fn("matmul");
  {
    Constant M(20);
    Constant K(10);
    Constant N(30);
    Expr A(cs({M, K}), primitive_t::float32);
    Expr B(cs({K, N}), primitive_t::float32);
    Expr C(cs({M, N}), primitive_t::float32);

    Var m, k, n;

    auto s0 = matmul_fn.AddStage(C[m][n].Assign(A[m][k] * B[k][n]));

    matmul_fn.Inputs({A, B});
    matmul_fn.Outputs({C});
    matmul_fn.EndDefinition();
  }

  LOG(INFO) << "matmul_fn: \n" << ir::Dump(matmul_fn);
}

}  // namespace cinn
