#pragma once

#include <string>
#include <vector>
#include "cinn/ir/ir.h"
#include "cinn/type.h"

namespace cinn {

// Buffer is used to store the result of computation and represent the argument of function.
class Buffer : public ir::Expr {
  bool allocated_{false};

  // Dimensions of this buffer.
  std::vector<ir::Expr> dims_;

  // Name of this buffer, should be unique.
  std::string name_;

  primitive_t ptype_{primitive_t::unk};

  memory_location_t memory_location_;

  argument_t arg_type_{argument_t::unknown};

 public:
  using dims_t = std::vector<Expr>;

  Buffer(const std::string& name, dims_t dims, primitive_t type, argument_t arg_type)
      : name_(name), dims_(dims), ptype_(type), arg_type_(arg_type) {}
};

}  // namespace cinn
