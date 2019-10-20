#include "cinn/backends/code_gen_llvm.h"
#include <gtest/gtest.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <fstream>
#include <memory>
#include "cinn/backends/llvm_jit.h"
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace backends {

TEST(code_gen_llvm, basic) {
  Target target;
  llvm::LLVMContext context;
  CodeGenLLVM gen(target, context, nullptr);

  ir::Expr a(1.f);
  ir::Expr b(2.f);

  ir::Expr c = a + b;
  auto res = gen.Codegen(c);

  res->print(llvm::errs());
}

TEST(code_gen_llvm, array) {
  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  ir::Expr c = ir::Expr(2.f) * 30.f + 1.1f;

  CodeGenLLVM gen(target, context, module.get());
  auto create_int_function = [&](const std::string& name, ir::Expr expr) {
    llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), /*not vararg*/ false);

    auto* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module.get());
    llvm::BasicBlock* block = llvm::BasicBlock::Create(context, "EntryBlock", f);

    llvm::IRBuilder<> builder(block);

    auto* add = gen.Codegen(expr);
    builder.CreateRet(add);
  };

  auto create_float_function = [&](const std::string& name, ir::Expr expr) {
    llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getFloatTy(context), /*not vararg*/ false);

    auto* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module.get());
    llvm::BasicBlock* block = llvm::BasicBlock::Create(context, "EntryBlock", f);

    llvm::IRBuilder<> builder(block);

    auto* add = gen.Codegen(expr);
    builder.CreateRet(add);
  };

  ir::Expr int_expr = ir::Expr(3) + 2 * 2008;
  create_int_function("int_fn", int_expr);

  ir::Expr float_expr = ir::Expr(2.3f) * 2.008f;
  create_float_function("float_fn", float_expr);

  auto jit = CreateJIT(module.get());
  jit->addModule(std::move(module));

  typedef int (*int_fn_t)();
  typedef float (*float_fn_t)();

  {
    auto int_fn = jit->findSymbol("int_fn");
    auto address = reinterpret_cast<int_fn_t>(int_fn.getAddress().get());
    llvm::outs() << "int function result: " << address() << "\n";
  }

  {
    auto fn = jit->findSymbol("float_fn");
    auto address = reinterpret_cast<float_fn_t>(fn.getAddress().get());
    llvm::outs() << "float function result: " << address() << "\n";
  }
}

TEST(code_gen_llvm, forloop) {
  ir::Var i("i");
  ir::Expr block = ir::Block::make({});
  ir::Expr forloop = ir::For::make(ir::Expr(0), i < 0, ir::Expr(1), block, i);

  Target target;
  llvm::LLVMContext context;
  llvm::Module module("test", context);

  llvm::FunctionType* ft = llvm::FunctionType::get(llvm::Type::getFloatTy(context), /*not vararg*/ false);

  auto* f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "main", &module);
  llvm::BasicBlock* llvm_block = llvm::BasicBlock::Create(context, "EntryBlock", f);

  CodeGenLLVM gen(target, context, &module);
  gen.builder_->SetInsertPoint(llvm_block);
  gen.function_ = f;

  llvm::Value* gen_forloop = gen.Codegen(forloop);
}

TEST(code_gen_llvm, function) {
  Function fn("fn0");
  {
    ir::Constant M("M", 20);
    ir::Var i("i");

    ir::Expr A({M}, primitive_t::float32, "A");
    fn.AddStage(A[i].Assign(ir::Expr(1.f)));

    fn.Inputs({A});
    fn.Outputs({A});
    fn.EndDefinition();
  }

  LOG(INFO) << "***** code: \n" << ir::Dump(fn.ir_function());

  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  CodeGenLLVM gen(target, context, module.get());
  gen.Visit(&fn.ir_function());

  auto jit = CreateJIT(module.get());
  llvm::outs().flush();
  llvm::outs() << "verify module: ";
  llvm::verifyModule(*module, &llvm::outs());
  llvm::outs().flush();

  module->print(llvm::outs(), nullptr);

  jit->addModule(std::move(module));

  auto symbol = jit->findSymbol("fn0");
  typedef void (*fn_t)(float*, float*);

  auto* fn_func = reinterpret_cast<fn_t>(symbol.getAddress().get());
  ASSERT_TRUE(fn_func);

  float A[20];
  for (int i = 0; i < 20; i++) A[i] = i;

  fn_func(A, A);

  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(A[i], 1.f);
  }
}

TEST(code_gen_llvm, function1) {
  Function fn("fn0");
  {
    ir::Constant M("M", 20);
    ir::Var i("i");

    ir::Expr A({M}, primitive_t::float32, "A");
    ir::Expr B({M}, primitive_t::float32, "B");

    fn.AddStage(A[i].Assign(ir::Expr(1.f)));
    fn.AddStage(B[i].Assign(ir::Expr(2.f)));

    fn.Inputs({A, B});
    fn.Outputs({});
    fn.EndDefinition();
  }

  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  CodeGenLLVM gen(target, context, module.get());
  gen.Visit(&fn.ir_function());

  auto jit = CreateJIT(module.get());
  llvm::outs().flush();
  llvm::outs() << "verify module: ";
  llvm::verifyModule(*module, &llvm::outs());
  llvm::outs().flush();

  module->print(llvm::outs(), nullptr);

  jit->addModule(std::move(module));

  auto symbol = jit->findSymbol("fn0");
  typedef void (*fn_t)(float*, float*);

  auto* fn_func = reinterpret_cast<fn_t>(symbol.getAddress().get());

  float A[20], B[20];

  fn_func(A, B);

  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(A[i], 1.f);
    ASSERT_EQ(B[i], 2.f);
  }
}

// A function with multiple Expr and assigment between each other.
// A[i] = 1
// B[i] = A[i] + 1
TEST(code_gen_llvm, function2) {
  Function fn("fn0");
  {
    ir::Constant M("M", 20);
    ir::Var i("i");

    ir::Expr A({M}, primitive_t::float32, "A");
    ir::Expr B({M}, primitive_t::float32, "B");

    fn.AddStage(A[i].Assign(ir::Expr(1.f)));
    fn.AddStage(B[i].Assign(A[i] + 1.f));

    fn.Inputs({A});
    fn.Outputs({B});
    fn.EndDefinition();

    ASSERT_TRUE(is_float(A[i].ptype()));
    ASSERT_TRUE(is_float(B[i].ptype()));
    ASSERT_TRUE(is_float((A[i] + 1.f).ptype()));
  }

  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  CodeGenLLVM gen(target, context, module.get());
  gen.Visit(&fn.ir_function());

  auto jit = CreateJIT(module.get());
  llvm::outs().flush();
  llvm::outs() << "verify module: ";
  llvm::verifyModule(*module, &llvm::outs());
  llvm::outs().flush();

  module->print(llvm::outs(), nullptr);

  jit->addModule(std::move(module));

  auto symbol = jit->findSymbol("fn0");
  typedef void (*fn_t)(float*, float*);

  auto* fn_func = reinterpret_cast<fn_t>(symbol.getAddress().get());

  float A[20], B[20];

  fn_func(A, B);

  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(A[i], 1.f);
    ASSERT_EQ(B[i], 2.f);
  }
}

// A[i] = 1
// B[i] = 2
// C[i] = A[i] * B[i]
TEST(code_gen_llvm, function3) {
  Function fn("fn0");
  {
    ir::Constant M("M", 20);
    ir::Var i("i");

    ir::Expr A({M}, primitive_t::float32, "A");
    ir::Expr B({M}, primitive_t::float32, "B");
    ir::Expr C({M}, primitive_t::float32, "C");

    fn.AddStage(A[i].Assign(ir::Expr(1.f)));
    fn.AddStage(B[i].Assign(A[i] + 1.f));
    fn.AddStage(C[i].Assign(A[i] * B[i] + 2.f));

    fn.Inputs({A, B});
    fn.Outputs({C});
    fn.EndDefinition();
  }

  Target target;
  llvm::LLVMContext context;
  std::unique_ptr<llvm::Module> module(new llvm::Module("test", context));

  CodeGenLLVM gen(target, context, module.get());
  gen.Visit(&fn.ir_function());

  auto jit = CreateJIT(module.get());
  llvm::outs().flush();
  llvm::outs() << "verify module: ";
  llvm::verifyModule(*module, &llvm::outs());
  llvm::outs().flush();

  module->print(llvm::outs(), nullptr);

  jit->addModule(std::move(module));

  auto symbol = jit->findSymbol("fn0");
  typedef void (*fn_t)(float*, float*, float*);

  auto* fn_func = reinterpret_cast<fn_t>(symbol.getAddress().get());

  float A[20], B[20], C[20];

  fn_func(A, B, C);

  for (int i = 0; i < 20; i++) {
    ASSERT_EQ(A[i], 1.f);
    ASSERT_EQ(B[i], 2.f);
    ASSERT_EQ(C[i], 4.f);
  }
}

}  // namespace backends
}  // namespace cinn