#pragma once
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

void BuildNetwork1(Network* net, Session* session) {
  using Var = Network::Var;

  std::vector<float> w0_data({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8});
  std::vector<float> b_data({0.1, 0.2});

  Var x0 = net->DeclInput("x0", primitive_t::float32, Shape{{3, 4}});
  Var w0 = net->DeclWeight<float>("w0", primitive_t::float32, Shape{{4, 2}}, w0_data.data());
  Var b = net->DeclWeight<float>("b", primitive_t::float32, Shape{{2}}, b_data.data());

  auto fc_out = net->AddFc(x0, w0, b);
  auto tanh_out = net->AddTanh(fc_out);

  net->DeclOutput(fc_out.name);
}

}  // namespace hlir
}  // namespace cinn
