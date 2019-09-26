#include "cinn/backends/code_gen_llvm.h"
#include "cinn/ir/ir.h"
#include "cinn/ir/ir_printer.h"

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

}  // namespace backends
}  // namespace cinn
