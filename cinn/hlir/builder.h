#pragma once
#include "cinn/hlir/graph.h"
#include "cinn/hlir/graph_util.h"
#include "cinn/hlir/network.h"

namespace cinn {
namespace hlir {

class Builder {
 public:
  //! Build a network.
  void Build(Session* session, Network&& net);
};

}  // namespace hlir
}  // namespace cinn
