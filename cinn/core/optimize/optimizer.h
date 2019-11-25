#pragma once
#include <cinn/ir/ir.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/optimize/pass.h"
#include "cinn/core/optimize/pass_registry.h"

namespace cinn {

/**
 * Base class for pass-like optimization modules.
 * @tparam T the element type to optimize such as ir::Expr or hlir::Graph.
 */
template <typename T>
class Optimizer {
 protected:
  std::vector<std::string> passes_in_order_;

 public:
  Optimizer(const std::vector<std::string>& passes) { passes_in_order_ = passes; }

  void operator()(T* expr) {
    for (auto& name : passes_in_order_) {
      auto* pass = PassRegistry<T>::Global().GetPass(name);
      CHECK(pass) << "pass [" << name << "] not exists";
      pass->Run(expr);
    }
  }

  void SetPasses(const std::vector<std::string>& passes) { passes_in_order_ = passes; }
};

/**
 * Optimizer for IR level.
 */
class IrOptimizer : public Optimizer<ir::Expr> {
 public:
  IrOptimizer(const std::vector<std::string>& passes =
                  {
                      "nested_block_clean",          //
                      "display_program",             //
                      "indices_to_absolute_offset",  //
                      "display_program",             //
                      "fold_reference_indices",      //
                      "display_program",             //
                      "vectorize",                   //
                      "nested_block_clean",          //
                      "display_program",             //
                      //"temp_variable_fold",          //
                      "unroll",           //
                      "display_program",  //
                  })
      : Optimizer(passes) {}
};

}  // namespace cinn
