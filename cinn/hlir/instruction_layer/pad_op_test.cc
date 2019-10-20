#include <gtest/gtest.h>
#include "cinn/hlir/instruction_layer/use_ops.h"
#include "cinn/hlir/op_registry.h"

namespace cinn {
namespace hlir {

TEST(pad, test) { auto op = OpRegistry::Global().CreateOp(HlirLayer::kInstructionWise, "pad"); }

}  // namespace hlir
}  // namespace cinn
