#pragma once
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

void BuildNetwork0(Network* net, Session* session) {
  net->AddMatMul("x0", "w", "y0");
  net->AddElementwise(Network::ElementwiseOpKind::kAdd, "y0", "b", "y1");
  net->AddTanh("y1", "y2");

  auto* x0 = session->NewTensor("x0");
  x0->set_shape({30, 40});

  auto* w = session->NewTensor("w");
  w->set_shape({40, 50});

  auto* b = session->NewTensor("b");
  b->set_shape({50});

  auto* y0 = session->NewTensor("y0");
  auto* y1 = session->NewTensor("y1");
  auto* y2 = session->NewTensor("y2");
}

}  // namespace hlir
}  // namespace cinn
