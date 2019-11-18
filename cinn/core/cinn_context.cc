#include "cinn/core/cinn_context.h"
#include "cinn/core/stage.h"

namespace cinn {

std::unique_ptr<CINNContext> _g_cinn_context;

void SetGlobalContext(CINNContext *context) {
  CHECK(context);
  _g_cinn_context.reset(context);
}

CINNContext &GlobalContext() {
  CHECK(_g_cinn_context) << "Set global context first";
  return *_g_cinn_context;
}

Generator::~Generator() {
  for (auto &item : stages_) {
    delete item.second;
  }
}

}  // namespace cinn
