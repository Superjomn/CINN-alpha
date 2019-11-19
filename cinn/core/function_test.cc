#include "cinn/core/function.h"
#include <gtest/gtest.h>
#include "cinn/core/stage.h"
#include "cinn/ir/ir_helper.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
using ir::Constant;
using ir::Expr;
using ir::Var;

using cs = std::vector<Constant>;

TEST(Function, basic) {
  SetGlobalContext(new CINNContext);

  Var i("i");
  Var j("j");

  Constant N(10), M(20), K(30);

  Function basic_fn("basic");
  {
    Expr A(cs({N, M}), primitive_t::float32, "A");
    Expr B(cs({M, K}), primitive_t::float32, "B");
    Expr C(cs({N, M}), primitive_t::float32, "C");

    Stage s0 = basic_fn.AddStage(C[i][j].Assign(A[i][j] * B[i][j]));
    Stage s1 = basic_fn.AddStage(C[i][j].Assign(C[i][j] + 1.f));

    basic_fn.Inputs({A, B});
    basic_fn.Outputs({C});

    basic_fn.EndDefinition();
  }

  LOG(INFO) << "func0 " << basic_fn.ir_function();
}

TEST(Function, buffer_allocate) {
  SetGlobalContext(new CINNContext);

  Var i("i");
  Var j("j");
  Var k("k");

  Constant M(100);
  Constant N(200);
  Constant K(300);

  Expr A(cs({M, K}), primitive_t::float32, "A");
  Expr B(cs({K, N}), primitive_t::float32, "B");
  Expr C(cs({M, N}), primitive_t::float32, "C");

  Function fn("fn");
  {
    auto s0 = fn.AddStage(C[i][j].Assign(A[i][j * 2] * B[i - 3][j]));
    auto s_alloc_D = fn.AddStage(ir::Allocate::make("D", Expr(10), primitive_t::float32));

    fn.Inputs({A, B});
    fn.Outputs({C});
    fn.EndDefinition();
  }

  LOG(INFO) << "func0.code : \n" << Expr(fn);
}

Expr tanh(Expr x) { return ir::Max::make(Expr(0.f), x); }

TEST(Snippet, test) {
  SetGlobalContext(new CINNContext);

  Snippet snippet0;

  Constant N(200);
  Constant M(100);

  Expr x(cs({N, M}), primitive_t::float32, "x");
  Expr y(cs({N, M}), primitive_t::float32, "y");

  Var i("i");
  Var j("j");

  Stage s0 = x[i][j].Assign(Expr(1.f));
  Stage s1 = y[i][j].Assign(x[i][j] + 1.f);

  snippet0.AddStage(s0);
  snippet0.AddStage(s1);
  snippet0.End();
}

TEST(Snippet, test1) {
  SetGlobalContext(new CINNContext);

  std::vector<Snippet> snippets;
  for (int i = 0; i < 10; i++) {
    snippets.emplace_back();
  }
}

TEST(Function, syntax) {
  SetGlobalContext(new CINNContext);

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

  LOG(INFO) << "tanh:\n" << tanh_fn.ir_function();

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

  LOG(INFO) << "softmax: \n" << softmax_fn.ir_function();

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

  LOG(INFO) << "matmul_fn: \n" << matmul_fn.ir_function();
}

TEST(Function, any_constant) {
  SetGlobalContext(new CINNContext);

  Function softmax_fn("softmax");
  {
    Constant N("N", primitive_t::int32);
    Constant D("D", primitive_t::int32);
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

  LOG(INFO) << "softmax: \n" << softmax_fn.ir_function();
}

TEST(Function, transform) {
  SetGlobalContext(new CINNContext);

  Function softmax_fn("softmax");
  {
    Constant N("N", 1000);
    Constant D("D", 2000);
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
    auto s2 = softmax_fn.AddStage(  //
        O[n][d] += Expr(1.f));

    // softmax_fn.EndDefinition();
  }

  softmax_fn.BuildSnippets(false);
  auto& snippets = softmax_fn.snippets();

  for (auto& snippet : snippets) {
    auto statements = const_cast<Snippet*>(&snippet)->CollectBandFirstStatement();
    for (auto& s : statements) {
      GlobalContext().generator().GetStageByName(s).TileUnroll({16, 16});
    }
  }

  softmax_fn.EndDefinition();

  LOG(INFO) << "softmax: " << std::endl << softmax_fn.ir_function() << std::endl;

  std::string target = R"ROC(def softmax (Tensor& var0, Tensor& var1, Tensor& var2) {
  for(c0, 0, (c0 <= 62), 16) {
    for(c1, 0, (c1 <= 124), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        for(c3, 0, (c3 <= 15), 1) {
          var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + c3)]));
        }
      }
    }
    for(c1, 128, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),c1]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 1)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 2)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 3)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 4)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 5)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 6)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 7)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 8)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 9)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 10)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 11)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 12)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 13)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 14)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 15)]));
      }
    }
  }
  for(c0, 64, (c0 <= 999), 16) {
    for(c1, 0, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= min(15,((-c0) + 999))), 1) {
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),c1]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 1)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 2)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 3)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 4)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 5)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 6)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 7)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 8)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 9)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 10)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 11)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 12)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 13)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 14)]));
        var1<1000>[(c0 + c2)] = (var1<1000>[(c0 + c2)] + exp(var0<1000,2000>[(c0 + c2),(c1 + 15)]));
      }
    }
  }
  for(c0, 0, (c0 <= 62), 16) {
    for(c1, 0, (c1 <= 124), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        for(c3, 0, (c3 <= 15), 1) {
          var2<1000,2000>[(c0 + c2),(c1 + c3)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + c3)]) / var1<1000>[(c0 + c2)]);
        }
      }
    }
    for(c1, 128, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        var2<1000,2000>[(c0 + c2),c1] = (exp(var0<1000,2000>[(c0 + c2),c1]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 1)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 1)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 2)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 2)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 3)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 3)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 4)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 4)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 5)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 5)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 6)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 6)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 7)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 7)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 8)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 8)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 9)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 9)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 10)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 10)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 11)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 11)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 12)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 12)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 13)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 13)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 14)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 14)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 15)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 15)]) / var1<1000>[(c0 + c2)]);
      }
    }
  }
  for(c0, 64, (c0 <= 999), 16) {
    for(c1, 0, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= min(15,((-c0) + 999))), 1) {
        var2<1000,2000>[(c0 + c2),c1] = (exp(var0<1000,2000>[(c0 + c2),c1]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 1)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 1)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 2)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 2)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 3)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 3)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 4)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 4)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 5)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 5)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 6)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 6)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 7)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 7)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 8)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 8)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 9)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 9)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 10)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 10)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 11)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 11)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 12)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 12)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 13)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 13)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 14)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 14)]) / var1<1000>[(c0 + c2)]);
        var2<1000,2000>[(c0 + c2),(c1 + 15)] = (exp(var0<1000,2000>[(c0 + c2),(c1 + 15)]) / var1<1000>[(c0 + c2)]);
      }
    }
  }
  for(c0, 0, (c0 <= 62), 16) {
    for(c1, 0, (c1 <= 124), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        for(c3, 0, (c3 <= 15), 1) {
          var2<1000,2000>[(c0 + c2),(c1 + c3)] += 1;
        }
      }
    }
    for(c1, 128, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= 15), 1) {
        var2<1000,2000>[(c0 + c2),c1] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 1)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 2)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 3)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 4)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 5)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 6)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 7)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 8)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 9)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 10)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 11)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 12)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 13)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 14)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 15)] += 1;
      }
    }
  }
  for(c0, 64, (c0 <= 999), 16) {
    for(c1, 0, (c1 <= 1999), 16) {
      for(c2, 0, (c2 <= min(15,((-c0) + 999))), 1) {
        var2<1000,2000>[(c0 + c2),c1] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 1)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 2)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 3)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 4)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 5)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 6)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 7)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 8)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 9)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 10)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 11)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 12)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 13)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 14)] += 1;
        var2<1000,2000>[(c0 + c2),(c1 + 15)] += 1;
      }
    }
  }
})ROC";

  EXPECT_EQ(GetStreamStr(softmax_fn.ir_function()), target);
}

}  // namespace cinn
