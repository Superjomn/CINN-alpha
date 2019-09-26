#include "cinn/backends/code_gen_llvm.h"
#include <gtest/gtest.h>
#include "cinn/ir/ir.h"

namespace cinn {
namespace backends {

TEST(code_gen_llvm, basic) {
  Target target;
  llvm::LLVMContext context;
  CodeGenLLVM gen(target, context);

  ir::Expr a(1.f);

  gen.Visit(&a);
}

}  // namespace backends
}  // namespace cinn