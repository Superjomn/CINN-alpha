#pragma once
namespace cinn {
namespace hlir {

enum class HlirLayer {
  kUnk = -1,
  kModelWise = 0,
  kMathlibWise,
  kInstructionWise,
  __NUM__,
};

}  // namespace hlir
}  // namespace cinn
