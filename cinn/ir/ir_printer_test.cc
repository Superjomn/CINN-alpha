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

TEST(IRPrinter, test1) {
  Expr a((float)0.1);
  Expr b((int)3);
  auto c = a > b;

  std::stringstream os;
  IRPrinter printer(os);
  printer.Visit(&c);
  auto log = os.str();
  LOG(INFO) << log;
  ASSERT_EQ(log, "(0.1 > 3)");
}

TEST(IRPrinter, block) {
  Expr a((float)0.1), b((int)1);
  Expr c = a > b;
  Expr c0 = a != b;
  Expr c1 = a + b;

  auto block = Block::make(std::vector<Expr>({c, c0, c1}));

  std::stringstream os;
  IRPrinter printer(os);
  printer.Print(block);

  auto log = os.str();
  LOG(INFO) << log;
}

TEST(IRPrinter, IfThenElse) {
  Expr a(0.1f);
  Expr b(0.2f);

  Expr x(100.f);
  Expr y(20.f);

  auto true_block = Block::make({x + y});
  auto false_block = Block::make({x - y});

  auto if_then_else = IfThenElse::make(a > b,  // condition
                                       true_block,
                                       false_block);

  std::stringstream os;
  IRPrinter printer(os);
  printer.Print(if_then_else);

  auto log = os.str();

  LOG(INFO) << "log:\n" << log;
}

TEST(IRPrinter, For) {
  Expr min(0);
  Expr extent(10);

  Expr x(100.f);
  Expr y(10.f);
  Expr body = Block::make({x + y});

  Var i("i");

  Expr for_ = For::make(min, extent, extent, body, i);

  std::stringstream os;
  IRPrinter printer(os);
  printer.Print(for_);
  auto log = os.str();

  LOG(INFO) << "log:\n" << log;
}

}  // namespace ir
}  // namespace cinn
