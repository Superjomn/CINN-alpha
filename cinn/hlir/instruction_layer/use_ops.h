#include "cinn/hlir/op_registry.h"

USE_OP(pad, kInstructionWise);
USE_OP(matmul, kInstructionWise);
USE_OP(reshape, kInstructionWise);

// elementwise operations
USE_OP(elementwise_add, kInstructionWise);
USE_OP(elementwise_sub, kInstructionWise);
USE_OP(elementwise_mul, kInstructionWise);
USE_OP(elementwise_div, kInstructionWise);

// activations.
USE_OP(tanh, kInstructionWise);
USE_OP(sigmoid, kInstructionWise);
