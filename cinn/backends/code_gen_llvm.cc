#include "cinn/backends/code_gen_llvm.h"
#include <glog/logging.h>
#include "cinn/core/function.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"
#include "cinn/utils/logging.h"

namespace cinn {
namespace backends {
void CodeGenLLVM::Visit(const ir::IntImm *op) {
  switch (op->ptype()) {
    case primitive_t::int8:
      value_ = llvm::ConstantInt::getSigned(i8_t, op->val());
      break;
    case primitive_t::int16:
      value_ = llvm::ConstantInt::getSigned(i16_t, op->val());
      break;
    case primitive_t::int32:
      value_ = llvm::ConstantInt::getSigned(i32_t, op->val());
      break;
    case primitive_t::int64:
      value_ = llvm::ConstantInt::getSigned(i16_t, op->val());
      break;
    default:
      LOG(FATAL) << "not supported type " << op->ptype();
  }
}

void CodeGenLLVM::Visit(const ir::FloatImm *op) {
  switch (op->ptype()) {
    case primitive_t::float32:
      value_ = llvm::ConstantFP::get(f32_t, op->val());
      break;
    case primitive_t::float64:
      value_ = llvm::ConstantFP::get(f64_t, op->val());
      break;
    default:
      LOG(FATAL) << "type not supported " << op->ptype();
  }
}

void CodeGenLLVM::Visit(const ir::Add *op) {
  if (is_float(op->ptype())) {
    value_ = builder_->CreateFAdd(Codegen(op->a), Codegen(op->b));
  } else if (is_integer(op->ptype())) {
    value_ = builder_->CreateNSWAdd(Codegen(op->a), Codegen(op->b));
  } else {
    LOG(INFO) << "ptype: " << op->ptype();
    value_ = builder_->CreateAdd(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::Sub *op) {
  if (is_float(op->ptype())) {
    value_ = builder_->CreateFSub(Codegen(op->a), Codegen(op->b));
  } else if (is_integer(op->ptype())) {
    value_ = builder_->CreateNSWSub(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateSub(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::Mul *op) {
  if (is_float(op->ptype())) {
    value_ = builder_->CreateFMul(Codegen(op->a), Codegen(op->b));
  } else if (is_integer(op->ptype())) {
    value_ = builder_->CreateNSWMul(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateMul(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::Div *op) {
  if (is_float(op->ptype())) {
    value_ = builder_->CreateFDiv(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateSDiv(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::Mod *op) {
  if (is_float(op->ptype())) {
    value_ = builder_->CreateFRem(Codegen(op->a), Codegen(op->b));
  } else {
    CHECK(is_integer(op->ptype()));
    value_ = builder_->CreateSRem(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::Min *op) {
  auto cond = ir::LT::make(op->a, op->b);
  value_ = builder_->CreateSelect(Codegen(cond), Codegen(op->a), Codegen(op->b));
}

void CodeGenLLVM::Visit(const ir::Max *op) {
  auto cond = ir::GT::make(op->a, op->b);
  value_ = builder_->CreateSelect(Codegen(cond), Codegen(op->a), Codegen(op->b));
}

void CodeGenLLVM::Visit(const ir::NE *op) {
  if (is_integer(op->a.ptype())) {
    value_ = builder_->CreateICmpNE(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpONE(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::EQ *op) {
  if (is_integer(op->a.ptype())) {
    value_ = builder_->CreateICmpEQ(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpOEQ(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::GT *op) {
  if (is_integer(op->a.ptype())) {
    // TODO(Superjomn) Consider the unsigned int condition.
    value_ = builder_->CreateICmpSGT(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpOGT(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::GE *op) {
  if (is_integer(op->a.ptype())) {
    // TODO(Superjomn) Consider the unsigned int condition.
    value_ = builder_->CreateICmpSGE(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpOGE(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::LT *op) {
  if (is_integer(op->a.ptype())) {
    // TODO(Superjomn) Consider the unsigned int condition.
    value_ = builder_->CreateICmpSLT(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpOLT(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::LE *op) {
  if (is_integer(op->a.ptype())) {
    // TODO(Superjomn) Consider the unsigned int condition.
    value_ = builder_->CreateICmpSLE(Codegen(op->a), Codegen(op->b));
  } else {
    value_ = builder_->CreateFCmpOLE(Codegen(op->a), Codegen(op->b));
  }
}

void CodeGenLLVM::Visit(const ir::And *op) { value_ = builder_->CreateAnd(Codegen(op->a), Codegen(op->b)); }

void CodeGenLLVM::Visit(const ir::Or *op) { value_ = builder_->CreateOr(Codegen(op->a), Codegen(op->b)); }

llvm::Value *CodeGenLLVM::Codegen(const ir::Expr &e) {
  CHECK(e.valid());
  ResetValue();
  Visit(&e);
  return value_;
}

llvm::Function *CodeGenLLVM::CreateFunctionPrototype(const Function *op) {
  // collect arguments
  std::vector<llvm::Type *> args(op->inputs().size() + op->outputs().size(), f32ptr_t);
  std::vector<Expr> _args;
  for (size_t i = 0; i < args.size(); i++) {
    _args.push_back((i < op->inputs().size()) ? op->inputs()[i] : op->outputs()[i - op->inputs().size()]);
  }

  // void fn(float* a, float* b...)
  llvm::FunctionType *function_type = llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx_), args, false);
  llvm::Function *function =
      llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, op->name(), module_);

  int i = 0;
  for (auto &f_arg : function->args()) {
    auto &arg = _args[i];
    CHECK(arg.is_tensor());
    auto name = arg.As<ir::Tensor>()->name();

    f_arg.setName(name);
  }

  return function;
}

void CodeGenLLVM::Visit(const Function *op) {
  LOG(INFO) << "to visit funciton";
  llvm::Function *function = module_->getFunction(op->name());
  CHECK(!function);
  // if (function) return;
  function = CreateFunctionPrototype(op);
  CHECK(function);
  CHECK(function->empty()) << "function " << op->name() << " is redefined";

  llvm::BasicBlock *block = llvm::BasicBlock::Create(*ctx_, "entry", function);
  builder_->SetInsertPoint(block);

  // prepare arguments
  std::vector<Expr> args(op->inputs().size() + op->outputs().size());
  auto *f_args = function->arg_begin();
  for (int i = 0; i < args.size(); i++) {
    std::string arg_name;
    if (i < op->inputs().size()) {
      CHECK(op->inputs()[i].is_tensor());
      LOG(INFO) << "called " << op->inputs()[i].As<ir::Tensor>()->name();
      f_args[i].setName(op->inputs()[i].As<ir::Tensor>()->name());
    } else {
      CHECK(op->outputs()[i - op->inputs().size()].is_tensor());
      LOG(INFO) << "called " << op->outputs()[i - op->inputs().size()].As<ir::Tensor>()->name();
      f_args[i].setName(op->outputs()[i - op->inputs().size()].As<ir::Tensor>()->name());
    }

    LOG(INFO) << "collect fn args: " << f_args[i].getName().str();
    fn_args_[f_args[i].getName()] = &f_args[i];
  }

  function_ = function;

  // prepare body.
  Visit(&op->ComputeTransformedExpr());

  auto *final_bb = llvm::BasicBlock::Create(*ctx_, "final", function_);
  builder_->CreateBr(final_bb);
  builder_->SetInsertPoint(final_bb);

  // void
  builder_->CreateRet(nullptr);

  // LOG(INFO) << "function: ";
  // llvm::outs() << "function: \n";
  // function->print(llvm::outs());
  // llvm::outs().flush();
  llvm::outs() << "******* verify function: ********"
               << "\n";
  llvm::verifyFunction(*function_, &llvm::outs());
  llvm::outs().flush();
}

void CodeGenLLVM::Visit(const ir::Assign *op) {
  // LHS should be a referenced tensor.
  CHECK(op->a.is_reference());
  const ir::Reference *ref = op->a.As<ir::Reference>();
  CHECK(ref->target.is_tensor());

  auto *lhs_array = Codegen(ref->target);
  CHECK_EQ(ref->iterators.size(), 1UL);
  auto *lhs_index = Codegen(ref->iterators.front());

  auto *lhs_ptr = builder_->CreateGEP(lhs_array, lhs_index, "p");
  auto *rhs_value = Codegen(op->b);

  auto *store = builder_->CreateStore(rhs_value, lhs_ptr);
}

void CodeGenLLVM::Visit(const ir::Constant *op) {
  // TODO(Superjomn) consider the i8, i16, and i64, so is the float points.
  if (op->is_integer()) {
    value_ = llvm::ConstantInt::getSigned(i32_t, op->As<int>());
  } else if (op->is_float()) {
    value_ = llvm::ConstantInt::getSigned(f32_t, op->As<float>());
  } else if (op->is_boolean()) {
    LOG(FATAL) << "not supported type " << op->ptype();
  }
}

// The Block expr should be carefully processed in the For and Function.
void CodeGenLLVM::Visit(const ir::Block *op) {
  for (auto &expr : op->exprs) {
    Visit(&expr);
  }
}

void CodeGenLLVM::Visit(const ir::Tensor *op) {
  CHECK(fn_args_.count(op->name())) << "fn_arg " << op->name() << "not exists";
  value_ = fn_args_[op->name()];
  value_->print(llvm::outs());
}

// read a reference.
void CodeGenLLVM::Visit(const ir::Reference *op) { ReadTensorElement(*op); }

void CodeGenLLVM::ReadTensorElement(const ir::Reference &ref) {
  auto *array_ptr = Codegen(ref.target);
  CHECK_EQ(ref.iterators.size(), 1UL) << "the indices should be an absolute offset";
  auto *index = Codegen(ref.iterators.front());

  auto *offset = builder_->CreateGEP(array_ptr, index);
  auto *element = builder_->CreateLoad(offset, false);
  value_ = element;
}

void CodeGenLLVM::Visit(const ir::For *op) {
  CHECK(builder_);
  LOG_INDENT("CodeGenLLVM::Visit");

  CINN_DEBUG(0) << "init: " << ir::Dump(op->iter_init);
  CINN_DEBUG(0) << "cond: " << ir::Dump(op->iter_cond);

  llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(*ctx_, "entry", function_);
  llvm::BasicBlock *cond_bb = llvm::BasicBlock::Create(*ctx_, "cond", function_);
  llvm::BasicBlock *loop_bb = llvm::BasicBlock::Create(*ctx_, "loop", function_);
  llvm::BasicBlock *inc_bb = llvm::BasicBlock::Create(*ctx_, "inc", function_);
  llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(*ctx_, "afterloop", function_);

  builder_->CreateBr(entry_bb);  // terminal the previous block.
  builder_->SetInsertPoint(entry_bb);
  llvm::Value *i_ptr = builder_->CreateAlloca(i32_t);
  {
    // llvm::kprintf(module_, builder_, "I am in %s", "entry block");
    // prepare entry block --
    // int i
    for_iterator_vars_[op->iterator.name()] = i_ptr;

    // i = 0;
    auto *init = Codegen(op->iter_init);
    builder_->CreateStore(init, i_ptr);
  }

  builder_->CreateBr(cond_bb);
  {
    builder_->SetInsertPoint(cond_bb);
    // cond bb
    llvm::Value *i = builder_->CreateLoad(i_ptr);
    for_iterator_vars_[op->iterator.name()] = i;
    llvm::Value *cond = Codegen(op->iter_cond);
    builder_->CreateCondBr(cond, loop_bb, after_bb);
  }

  // builder_->CreateBr(loop_bb);
  {
    builder_->SetInsertPoint(loop_bb);

    auto *load = builder_->CreateLoad(i_ptr);
    // llvm::kprintf(module_, builder_, "loop %d\n", load);
    // visit body
    Visit(&op->body);
  }

  builder_->CreateBr(inc_bb);
  {
    builder_->SetInsertPoint(inc_bb);
    auto *i = builder_->CreateLoad(i_ptr, "i");
    auto *inc = Codegen(op->iter_inc);
    // auto* inc = llvm::ConstantInt::get(i32_t, 1);
    auto *add = builder_->CreateAdd(i, inc);
    builder_->CreateStore(add, i_ptr);
  }

  builder_->CreateBr(cond_bb);
  { builder_->SetInsertPoint(after_bb); }
}

void CodeGenLLVM::Visit(const ir::Var *op) {
  CHECK(for_iterator_vars_.count(op->name()));
  auto *loop_var = for_iterator_vars_[op->name()];
  value_ = loop_var;
}

CodeGenLLVM::CodeGenLLVM(Target target, llvm::LLVMContext &ctx, llvm::Module *module)
    : target_(target), ctx_(&ctx), module_(module), IRPrinter(std::cerr) {
  CHECK(ctx_);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // element type
  void_t = llvm::Type::getVoidTy(*ctx_);
  i8_t = llvm::Type::getInt8Ty(*ctx_);
  i16_t = llvm::Type::getInt16Ty(*ctx_);
  i32_t = llvm::Type::getInt32Ty(*ctx_);
  i64_t = llvm::Type::getInt64Ty(*ctx_);
  f16_t = llvm::Type::getHalfTy(*ctx_);
  f32_t = llvm::Type::getFloatTy(*ctx_);
  f64_t = llvm::Type::getDoubleTy(*ctx_);

  // pointer type
  f16ptr_t = llvm::Type::getFloatPtrTy(*ctx_, 16);
  f32ptr_t = llvm::Type::getFloatPtrTy(*ctx_, 32);
  f64ptr_t = llvm::Type::getFloatPtrTy(*ctx_, 64);
  i32ptr_t = llvm::Type::getInt32Ty(*ctx_);

  builder_ = new llvm::IRBuilder<>(*ctx_);
}

}  // namespace backends
}  // namespace cinn
