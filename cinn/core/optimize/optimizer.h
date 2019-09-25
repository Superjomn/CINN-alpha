#pragma once
#include <cinn/ir/ir.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/optimize/pass.h"

namespace cinn {

class Optimizer {
  std::vector<std::string> passes_in_order_;

 public:
  Optimizer();

  void operator()(ir::Expr* expr);

  void SetPasses(const std::vector<std::string>& passes) { passes_in_order_ = passes; }
};

}  // namespace cinn
