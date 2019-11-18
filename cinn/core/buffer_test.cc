#include "cinn/core/buffer.h"
#include <gtest/gtest.h>
#include "cinn/ir/ir_printer.h"

namespace cinn {

TEST(buffer, create) {
  SetGlobalContext(new CINNContext);

  Target target(Target::OS::Linux);
  ir::Expr size(100);
  Buffer buffer(target, size, primitive_t::float32);
  std::string gen = ir::Dump(buffer.create_expr());
  LOG(INFO) << gen;
  auto to = StringFormat("%s = create_buffer()", buffer.name().c_str());
  ASSERT_EQ(gen, to);
}

TEST(buffer, destroy) {
  SetGlobalContext(new CINNContext);

  Target target(Target::OS::Linux);
  ir::Expr size(100);
  Buffer buffer(target, size, primitive_t::float32);
  buffer.create_expr();
  std::string gen = ir::Dump(buffer.destroy_expr());
  LOG(INFO) << gen;
  auto to = StringFormat("destroy_buffer(%s)", buffer.name().c_str());
  ASSERT_EQ(gen, to);
}

}  // namespace cinn
