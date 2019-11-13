#pragma once
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

void BuildNetwork0(Network* net, Session* session) {
  using Var = Network::Var;
  auto mul_out = net->AddMatMul(Var("x0"), Var("w"));
  auto ele_out = net->AddElementwise(Network::ElementwiseOpKind::kAdd, mul_out, Var("b"));
  auto tanh_out = net->AddTanh(Var(ele_out));

  auto* x0 = session->NewTensor("x0");
  x0->set_shape({30, 40});

  auto* w = session->NewTensor("w");
  w->set_shape({40, 50});

  auto* b = session->NewTensor("b");
  b->set_shape({50});
}

}  // namespace hlir
}  // namespace cinn
