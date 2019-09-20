#include "cinn/module.h"
#include "cinn/core/stage.h"
#include "cinn/utils/logging.h"

namespace cinn {

void Module::AddFunction(const Expr &function) { functions_.push_back(function); }

void Module::Lower() {
  LOG_INDENT("Module::Lower");
  for (auto &function : functions_) {
    CINN_DEBUG(2) << "lower function " << function.As<Function>()->name();
    LowerFunction(function);
  }
}

void Module::Dump() {}

Module Module::LinkModules(const std::string &name, const std::vector<Module> &modules) {}

class AssignTargetNameCollector : public ir::IRVisitor {
 public:
  AssignTargetNameCollector(std::set<std::string> *names) : names_(names) {}

  void Visit(const ir::Reference *op) override {
    CHECK(op->target.is_var());
    names_->insert(op->target.As<ir::Var>()->name());
  }

  void Visit(const Expr *op) override { IRVisitor::Visit(op); }

 private:
  std::set<std::string> *names_;
};

std::set<std::string> FindoutTemporaryBuffer(Function &function) {
  LOG_INDENT("FindoutTemporaryBuffer");
  std::set<std::string> io_names, buf_names;
  AssignTargetNameCollector io_collector(&io_names), buf_collector(&buf_names);

  for (const Expr &x : function.inputs()) {
    io_collector.Visit(&x);
  }
  for (const Expr &x : function.outputs()) {
    io_collector.Visit(&x);
  }

  for (auto &stage : function.stages()) {
    buf_collector.Visit(&stage.expr());
  }

  CINN_DEBUG(3) << "get ios.size " << io_names.size();
  CINN_DEBUG(3) << "get bufs.size " << buf_names.size();

  std::set<std::string> result;
  for (auto &x : buf_names) {
    if (!io_names.count(x)) result.insert(x);
  }
  return result;
}

void PreAppendBufAllocateInFunction(Function *function, const std::string &buf_name) {}

void Module::LowerFunction(Expr &function) {
  CHECK(function.is_function());
  Function *func = function.As<Function>();

  Module::BufferDTypeInference(func);
  Module::BufferSizeInference(func);

  // Find out the temporary buffer used.
  std::set<std::string> tmp_buf_names = FindoutTemporaryBuffer(*func);
}

}  // namespace cinn
