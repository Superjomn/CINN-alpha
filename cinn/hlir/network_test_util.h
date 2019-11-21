#pragma once
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

struct Network1Builder {
  std::vector<float> w0_data{{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8}};
  std::vector<float> b_data{{0.1, 0.2}};

  Shape x0_shape{{3, 4}};
  Shape w0_shape{{4, 2}};
  Shape b_shape{{2}};

  void Build(Network* net, Session* session) {
    using Var = Network::Var;

    Var x0 = net->DeclInput("x0", primitive_t::float32, x0_shape);
    Var w0 = net->DeclWeight<float>("w0", primitive_t::float32, w0_shape, w0_data);
    Var b = net->DeclWeight<float>("b", primitive_t::float32, b_shape, b_data);

    auto fc_out = net->AddFc(x0, w0, b, false);
    auto tanh_out = net->AddTanh(fc_out);

    net->DeclOutput(fc_out.name);
  }
};

struct Network2Builder {
  using Var = Network::Var;

  const int layers;
  bool matmul_transposed{};

  const int dim = 128;

  Network2Builder(int layers, bool matmul_transposed = false) : layers(layers), matmul_transposed(matmul_transposed) {}

  Shape x0_shape{{10, dim}};
  Shape w0_shape{{dim, dim}};
  Shape b_shape{{dim}};

  std::vector<float> w0_data;
  std::vector<float> b_data;

  void Build(Network* net, Session* session) {
    InitInputs();

    Var x0 = net->DeclInput("x0", primitive_t::float32, x0_shape);
    Var w0 = net->DeclWeight<float>("w0", primitive_t::float32, w0_shape, w0_data);
    Var b = net->DeclWeight<float>("b", primitive_t::float32, b_shape, b_data);

    for (int i = 0; i < layers; i++) {
      x0 = AddLayer(net, x0, w0, b);
    }

    net->DeclOutput(x0.name);
  }

  std::vector<float> ManualTest(const std::vector<float>& x) {
    InitInputs();

    auto x_copied = x;
    for (int i = 0; i < layers; i++) {
      x_copied = AddManualTestLayer(x_copied);
    }
    return x_copied;
  }

  std::vector<float> AddManualTestLayer(const std::vector<float>& x) {
    const int M = x0_shape[0];
    const int N = w0_shape[1];
    const int K = w0_shape[0];

    std::vector<float> y(M * N, 0.f);

    for (int i = 0; i < M; i++) {
      for (int j = 0; j < N; j++) {
        for (int k = 0; k < K; k++) {
          // y[i][j] += x[i][k] * w[k][j]
          y[i * N + j] += x[i * K + k] * w0_data[k * N + j];
        }

        y[i * N + j] += b_data[j];
        y[i * N + j] = std::max(y[i * N + j], 0.f);
      }
    }
    return y;
  }

  Var AddLayer(Network* net, Var x, Var w0, Var b) {
    auto fc_out = net->AddFc(x, w0, b, matmul_transposed);
    auto tanh_out = net->AddTanh(fc_out);
    return tanh_out;
  }

  void InitInputs() {
    InitW();
    InitB();
  }

  void InitW() {
    w0_data.resize(w0_shape.num_elements());

    for (int i = 0; i < w0_data.size(); i++) {
      w0_data[i] = 0.001 * i;
    }
  }

  void InitB() {
    b_data.resize(b_shape.num_elements());

    for (int i = 0; i < b_data.size(); i++) {
      b_data[i] = 0.01 * i;
    }
  }
};

}  // namespace hlir
}  // namespace cinn
