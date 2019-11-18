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

    auto fc_out = net->AddFc(x0, w0, b);
    auto tanh_out = net->AddTanh(fc_out);

    net->DeclOutput(fc_out.name);
  }
};

}  // namespace hlir
}  // namespace cinn
