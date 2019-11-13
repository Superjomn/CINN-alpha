#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

std::unique_ptr<Operator> OpRegistry::CreateOp(HlirLayer layer, const std::string &type) {
  CHECK_EQ(registry_.size(), static_cast<int>(HlirLayer::__NUM__));
  auto &registry = registry_[static_cast<int>(layer)];
  auto creator_it = registry.find(type);
  CHECK(creator_it != std::end(registry)) << type;
  return creator_it->second();
}

void OpRegistry::RegistorOp(HlirLayer layer, const std::string &type, OpRegistry::op_creator &&creator) {
  CHECK(layer != HlirLayer::kUnk);
  registry_[static_cast<int>(layer)].emplace(type, std::move(creator));
}

}  // namespace hlir
}  // namespace cinn