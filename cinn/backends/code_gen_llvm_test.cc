#include "cinn/backends/code_gen_llvm.h"
#include <gtest/gtest.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <memory>
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace backends {

TEST(code_gen_llvm, basic) {
  Target target;
  llvm::LLVMContext context;
  CodeGenLLVM gen(target, context);

  ir::Expr a(1.f);
  ir::Expr b(2.f);

  ir::Expr c = a + b;
  auto res = gen.Codegen(c);

  res->print(llvm::errs());
}

}  // namespace backends
}  // namespace cinn