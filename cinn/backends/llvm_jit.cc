#include "cinn/backends/llvm_jit.h"

namespace llvm {
namespace orc {

KaleidoscopeJIT::ModuleHandleT KaleidoscopeJIT::addModule(std::unique_ptr<Module> M) {
  // We need a memory manager to allocate memory and resolve symbols for this
  // new module. Create one that resolves symbols by looking back into the
  // JIT.
  auto Resolver = createLambdaResolver(
      [&](const std::string &Name) {
        if (auto Sym = findMangledSymbol(Name)) return Sym;
        return JITSymbol(nullptr);
      },
      [](const std::string &S) { return nullptr; });
  auto H = cantFail(CompileLayer.addModule(std::move(M), std::move(Resolver)));

  ModuleHandles.push_back(H);
  return H;
}

void KaleidoscopeJIT::removeModule(KaleidoscopeJIT::ModuleHandleT H) {
  ModuleHandles.erase(find(ModuleHandles, H));
  cantFail(CompileLayer.removeModule(H));
}

std::string KaleidoscopeJIT::mangle(const std::string &Name) {
  std::string MangledName;
  {
    raw_string_ostream MangledNameStream(MangledName);
    Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
  }
  return MangledName;
}

}  // namespace orc
}  // namespace llvm

namespace cinn {
namespace backends {

std::unique_ptr<llvm::orc::KaleidoscopeJIT> CreateJIT(llvm::Module *module) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  std::unique_ptr<llvm::orc::KaleidoscopeJIT> jit(new llvm::orc::KaleidoscopeJIT);

  module->setDataLayout(jit->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  auto fpm = llvm::make_unique<llvm::legacy::FunctionPassManager>(module);

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  fpm->add(llvm::createInstructionCombiningPass());
  // Reassociate expressions.
  fpm->add(llvm::createReassociatePass());
  // Eliminate Common SubExpressions.
  fpm->add(llvm::createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  fpm->add(llvm::createCFGSimplificationPass());

  fpm->doInitialization();

  // Add the current debug info version into the module.
  module->addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);

  // Darwin only supports dwarf2.
  if (llvm::Triple(llvm::sys::getProcessTriple()).isOSDarwin())
    module->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);

  // Construct the DIBuilder, we do this here because we need the module.
  // auto DBuilder = llvm::make_unique<DIBuilder>(*M);

  return jit;
}
}  // namespace backends
}  // namespace cinn
