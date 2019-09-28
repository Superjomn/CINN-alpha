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
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value*> NamedValues;
static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
static std::unique_ptr<KaleidoscopeJIT> TheJIT;
static std::unique_ptr<DIBuilder> DBuilder;

struct DebugInfo {
  DICompileUnit* TheCU;
  DIType* DblTy;
  std::vector<DIScope*> LexicalBlocks;

  DIType* getDoubleTy();
} KSDbgInfo;

static void InitializeModuleAndPassManager() {
  // Open a new module.
  TheModule = llvm::make_unique<Module>("my cool jit", TheContext);
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<legacy::FunctionPassManager>(TheModule.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
}

llvm::Function* getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto* F = TheModule->getFunction(Name)) return F;
}

TEST(jit, basic) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  std::unique_ptr<KaleidoscopeJIT> TheJIT(new KaleidoscopeJIT);

  // initialize module
  std::unique_ptr<llvm::Module> TheModule(new llvm::Module("my module", TheContext));
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<legacy::FunctionPassManager>(TheModule.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();

  auto* M = TheModule.get();

  // Add the current debug info version into the module.
  TheModule->addModuleFlag(Module::Warning, "Debug Info Version", DEBUG_METADATA_VERSION);

  // Darwin only supports dwarf2.
  if (Triple(sys::getProcessTriple()).isOSDarwin()) TheModule->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);

  // Construct the DIBuilder, we do this here because we need the module.
  DBuilder = llvm::make_unique<DIBuilder>(*TheModule);

  // Create the compile unit for the module.
  // Currently down as "fib.ks" as a filename since we're redirecting stdin
  // but we'd like actual source locations.
  KSDbgInfo.TheCU = DBuilder->createCompileUnit(
      dwarf::DW_LANG_C, DBuilder->createFile("fib.ks", "."), "Kaleidoscope Compiler", 0, "", 0);

  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

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

  auto* main_fn = TheModule->getFunction("abs");
  LOG(INFO) << "main_fn ";
  main_fn->print(errs());

  TheJIT->addModule(std::move(TheModule));
  // InitializeModuleAndPassManager();

  auto symbol = TheJIT->findSymbol("abs");

  typedef int (*fn_t)();

  auto address = reinterpret_cast<fn_t>(symbol.getAddress().get());
  LOG(INFO) << "address: " << address;
  outs() << address();
}
