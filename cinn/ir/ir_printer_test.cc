#include "cinn/ir/ir_printer.h"
#include <gtest/gtest.h>
#include <sstream>
#include "cinn/ir/ir.h"
#include "cinn/ir/ops_overload.h"

namespace cinn {
namespace ir {

TEST(IRPrinter, basic) {
  Expr a(1);
  Expr b(2);
  Expr d(1.2f);

  Expr c = (a + b);
  Expr e = c * d;
  std::stringstream ss;

  IRPrinter printer(ss);
  printer.Visit(&e);

  auto log = ss.str();
  ASSERT_EQ(log, "((1 + 2) * 1.2)");

  LOG(INFO) << "log: " << log;
}

}  // namespace ir
}  // namespace cinn
