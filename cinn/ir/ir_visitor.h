#pragma once

namespace cinn {
namespace ir {

struct Stmt;
struct Add;
struct Sub;
struct Mul;
struct IntImm;
struct FloatImm;

/// Visitor pattern for IR nodes.
class IRVisitor {
 public:
  IRVisitor() = default;

  void visit(const Stmt*);
  void visit(const Add*);
  void visit(const Sub*);
  void visit(const Mul*);
  void visit(const IntImm*);
  void visit(const FloatImm*);
};

}  // namespace ir
}  // namespace cinn
