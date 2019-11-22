#pragma once
#include "cinn/core/optimize/pass_registry.h"

USE_IR_PASS(nested_block_clean);
USE_IR_PASS(indices_to_absolute_offset);
USE_IR_PASS(fold_reference_indices);
USE_IR_PASS(vectorize);
USE_IR_PASS(display_program);
USE_IR_PASS(call_once_process);
