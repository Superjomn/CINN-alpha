#include "cinn/backends/llvm_jit.h"
#include <glog/logging.h>
#include <gtest/gtest.h>
#include "cinn/backends/llvm_headers.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

using namespace llvm;
using namespace llvm::orc;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);

struct DebugInfo {
  DICompileUnit* TheCU;
  DIType* DblTy;
  std::vector<DIScope*> LexicalBlocks;

  DIType* getDoubleTy();
} KSDbgInfo;

TEST(jit, basic) {
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("my model", context));
  auto* M = module.get();

  auto jit = cinn::backends::CreateJIT(M);

  // main function
  // @{
  FunctionType* ft = FunctionType::get(Type::getInt32Ty(TheContext), /*not vararg*/ false);

  Function* f = Function::Create(ft, Function::ExternalLinkage, "abs", M);
  BasicBlock* block = BasicBlock::Create(TheContext, "EntryBlock", f);

  Value* Two = ConstantInt::get(Type::getInt32Ty(TheContext), 2);
  Value* Three = ConstantInt::get(Type::getInt32Ty(TheContext), 3);

  Instruction* Add = BinaryOperator::Create(Instruction::Add, Two, Three, "add");

  block->getInstList().push_back(Add);
  block->getInstList().push_back(ReturnInst::Create(TheContext, Add));
  //@}

  auto* main_fn = M->getFunction("abs");
  LOG(INFO) << "main_fn ";
  main_fn->print(errs());

  jit->addModule(std::move(module));

  auto symbol = jit->findSymbol("abs");

  typedef int (*fn_t)();

  auto address = reinterpret_cast<fn_t>(symbol.getAddress().get());
  LOG(INFO) << "address: " << address;
  outs() << address();
}

TEST(llvm, function_with_loop) {
  LLVMContext context;
  std::unique_ptr<Module> module(new Module("test", context));
  auto* M = module.get();

  FunctionType* ft = FunctionType::get(Type::getInt32Ty(context), /*not vararg*/ false);

  Function* f = Function::Create(ft, Function::ExternalLinkage, "foo", M);

  // int a = 0;
  // for (int i = 0; i < 100; i ++) {
  //  a += 1;
  //}
  // forloop @{
  // @}
  auto* int_ty = llvm::Type::getInt32Ty(context);

  auto* entry_bb = BasicBlock::Create(context, "entry", f);
  auto* cond_bb = BasicBlock::Create(context, "cond", f);
  auto* loop_bb = BasicBlock::Create(context, "loop", f);
  auto* inc_bb = BasicBlock::Create(context, "loop", f);
  auto* after_loop_bb = BasicBlock::Create(context, "after_loop_bb", f);

  IRBuilder<> builder(context);
  builder.SetInsertPoint(entry_bb);
  // declare a and i
  auto* a_ptr = builder.CreateAlloca(int_ty);
  auto* i_ptr = builder.CreateAlloca(int_ty);
  builder.CreateStore(ConstantInt::get(int_ty, 0), a_ptr, false);  // a = 0
  builder.CreateStore(ConstantInt::get(int_ty, 0), i_ptr, false);  // i = 0

  builder.CreateBr(cond_bb);

  // cond: bb i < 100
  builder.SetInsertPoint(cond_bb);
  auto* int_100 = ConstantInt::get(int_ty, 100);
  auto* int_1 = ConstantInt::get(int_ty, 1);
  auto* i = builder.CreateLoad(i_ptr, "i");
  auto* cond = builder.CreateICmpSLT(i, int_100);
  builder.CreateCondBr(cond, loop_bb, after_loop_bb);

  // loop br: a += i
  builder.SetInsertPoint(loop_bb);
  auto* a = builder.CreateLoad(a_ptr, "a");
  auto* ii = builder.CreateLoad(i_ptr, "i");
  auto* add = builder.CreateAdd(a, ii);
  builder.CreateStore(add, a_ptr);
  builder.CreateBr(inc_bb);

  // inc bb
  builder.SetInsertPoint(inc_bb);
  auto* iii = builder.CreateLoad(i_ptr, "i");
  auto* add1 = builder.CreateAdd(iii, int_1);
  builder.CreateStore(add1, i_ptr);
  builder.CreateBr(cond_bb);

  // after loop
  builder.SetInsertPoint(after_loop_bb);
  auto* aa = builder.CreateLoad(a_ptr, "a");
  builder.CreateRet(aa);

  verifyFunction(*f, &outs());
  verifyModule(*M, &outs());

  auto jit = cinn::backends::CreateJIT(M);
  jit->addModule(std::move(module));

  typedef int (*fn_t)();
  auto symbol = jit->findSymbol("foo");
  auto* foo = reinterpret_cast<fn_t>(symbol.getAddress().get());
  LOG(INFO) << "get result: " << foo();
}

TEST(llvm, external_call) {
  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  llvm::IRBuilder<> builder(context);

  llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getVoidTy(context), /*not vararg*/ false);
  auto* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "foo", module.get());
  llvm::BasicBlock* block = llvm::BasicBlock::Create(context, "EntryBlock", f);
  builder.SetInsertPoint(block);

  llvm::kprintf(module.get(), &builder, "printf works: %s\n", "world");
  llvm::kprintf(module.get(), &builder, "haha works: %s\n", "Superjomn");
  llvm::kprintf(module.get(), &builder, "haha %d", llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1));

  builder.CreateRet(nullptr);

  std::error_code EC;
  llvm::raw_fd_ostream OS("file.ll", EC, llvm::sys::fs::F_None);
  llvm::WriteBitcodeToFile(module.get(), OS);
  OS.flush();

  auto jit = cinn::backends::CreateJIT(module.get());
  jit->addModule(std::move(module));
  auto symbol = jit->findSymbol("foo");

  typedef void (*fn_t)();
  fn_t fn = reinterpret_cast<fn_t>(symbol.getAddress().get());
  fn();
}
