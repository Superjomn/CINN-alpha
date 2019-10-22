#pragma once
#include "cinn/core/optimize/optimizer.h"
#include "cinn/hlir/graph.h"

namespace cinn {
namespace hlir {
namespace optimize {

/**
 * Optimizer for HLIR graph.
 */
class HlirOptimizer : public Optimizer<Graph> {
 public:
  HlirOptimizer() : Optimizer({"graph_to_ir_functions"}) {}
};

}  // namespace optimize
}  // namespace hlir
}  // namespace cinn
