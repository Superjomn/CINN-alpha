/**
 * A helper class to make the pass's registry easier.
 */
#pragma once
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "cinn/core/optimize/optimizer.h"

#define REGISTER_PASS(name__, T)                   \
  ::cinn::PassRegistor<T> name__##pass__(#name__); \
  int pass_touch_##name__() {                      \
    name__##pass__.Touch();                        \
    return 1;                                      \
  }
#define USE_PASS(name__)            \
  extern int pass_touch_##name__(); \
  int pass_##name__##_fake __attribute__((unused)) = pass_touch_##name__();

namespace cinn {

class PassRegistry {
  std::map<std::string, std::unique_ptr<Pass>> data_;

 public:
  static PassRegistry& Global() {
    static PassRegistry x;
    return x;
  }

  Pass* RegisterPass(const std::string& name, std::unique_ptr<Pass>&& x) {
    data_[name] = std::move(x);
    return data_[name].get();
  }

  Pass* GetPass(const std::string& name) {
    auto it = data_.find(name);
    if (it != data_.end()) return it->second.get();
    return nullptr;
  }
};

template <typename T>
struct PassRegistor {
  explicit PassRegistor(const std::string& name) {
    PassRegistry::Global().RegisterPass(name, std::unique_ptr<Pass>(new T(name)));
  }

  void Touch() {}
};

}  // namespace cinn
