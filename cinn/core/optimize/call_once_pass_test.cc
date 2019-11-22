#include <gtest/gtest.h>
#include "cinn/core/optimize/optimizer.h"
#include "cinn/core/optimize/use_passes.h"
#include "cinn/ir/ir_helper.h"

namespace cinn {

TEST(call_once_pass, test) {
  SetGlobalContext(new CINNContext);

  IrOptimizer optimizer({"call_once_process"});

  ir::Expr a("a", primitive_t::float32);
  ir::Expr b("b", primitive_t::float32);
  ir::Expr c("c", primitive_t::float32);

  ir::Expr expr = ir::Assign::make(c, a + b);

  ir::Expr block = ir::Block::make({expr});

  expr.Reset(ir::CallOnce::make(block));

  ir::Expr data_block = ir::Block::make({});
  ir::Expr function_block = ir::Block::make({expr});

  ir::Expr module = ir::Module::make(data_block, function_block);

  LOG(INFO) << std::endl << module;

  optimizer(&module);

  LOG(INFO) << "after optimizer:" << std::endl << module;

  std::string target = R"ROC(primitive boolean tmp0 = 1;

if(tmp0) {
  c = (a + b);
  tmp0 = 0;
}
)ROC";

  ASSERT_EQ(GetStreamStr(module), target);
}

}  // namespace cinn