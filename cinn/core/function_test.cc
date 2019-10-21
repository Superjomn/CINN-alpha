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

  LOG(INFO) << "func0 " << ir::Dump(basic_fn);
}

TEST(Function, buffer_allocate) {
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

  LOG(INFO) << "func0.code : \n" << ir::Dump(Expr(fn));
}

Expr tanh(Expr x) { return ir::Max::make(Expr(0.f), x); }

TEST(Snippet, test) {
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
