#include "cinn/backends/llvm_headers.h"
#include <gtest/gtest.h>

namespace cinn {
namespace backends {
using namespace llvm;

TEST(LLVM, basic) {
  LLVMContext Context;
  auto* M = new Module("test", Context);

  FunctionType* ft = FunctionType::get(Type::getInt32Ty(Context), /*not vararg*/ false);

  Function* f = Function::Create(ft, Function::ExternalLinkage, "main", M);
  BasicBlock* block = BasicBlock::Create(Context, "EntryBlock", f);

  Value* Two = ConstantInt::get(Type::getInt32Ty(Context), 2);
  Value* Three = ConstantInt::get(Type::getInt32Ty(Context), 3);

  Instruction* Add = BinaryOperator::Create(Instruction::Add, Two, Three, "add");

  block->getInstList().push_back(Add);
  block->getInstList().push_back(ReturnInst::Create(Context, Add));

  WriteBitcodeToFile(*M, outs());
}

}  // namespace backends
}  // namespace cinn
