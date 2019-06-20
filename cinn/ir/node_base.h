#pragma once
#include <memory>

namespace cinn {
namespace ir {

/// NodeBase is the base of all the nodes, including expressions and variables.
class NodeBase {
 public:
};

using NodePtr = std::shared_ptr<NodeBase>;

}  // namespace ir
}  // namespace cinn
