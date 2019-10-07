#pragma once

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/ConstantFolder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/DataExtractor.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Instrumentation.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/Transforms/Utils/SymbolRewriter.h>
#include <cstdarg>
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

static void kprintf(llvm::Module* module, llvm::IRBuilder<>* builder, const char* format, ...) {
  using namespace llvm;
  llvm::Function* func_printf = module->getFunction("printf");
  if (!func_printf) {
    auto* pty = llvm::PointerType::get(IntegerType::get(module->getContext(), 8), 0);
    FunctionType* func_ty = FunctionType::get(IntegerType::get(module->getContext(), 32), true);

    func_printf = llvm::Function::Create(func_ty, GlobalValue::ExternalLinkage, "printf", module);
  }

  Value* str = builder->CreateGlobalStringPtr(format);
  std::vector<Value*> int32_call_params;
  int32_call_params.push_back(str);

  va_list ap;
  va_start(ap, format);

  char* str_ptr = va_arg(ap, char*);
  Value* format_ptr = builder->CreateGlobalStringPtr(str_ptr);
  int32_call_params.push_back(format_ptr);

  builder->CreateCall(func_printf, int32_call_params, "");
}

static void kprintf(llvm::Module* module, llvm::IRBuilder<>* builder, const char* format, llvm::Value* i) {
  using namespace llvm;
  llvm::Function* func_printf = module->getFunction("printf");
  if (!func_printf) {
    auto* pty = llvm::PointerType::get(IntegerType::get(module->getContext(), 8), 0);
    FunctionType* func_ty = FunctionType::get(IntegerType::get(module->getContext(), 32), true);

    func_printf = llvm::Function::Create(func_ty, GlobalValue::ExternalLinkage, "printf", module);
  }

  Value* str = builder->CreateGlobalStringPtr(format);
  std::vector<Value*> int32_call_params;
  int32_call_params.push_back(str);
  int32_call_params.push_back(i);

  builder->CreateCall(func_printf, int32_call_params, "");
}
}  // namespace llvm
