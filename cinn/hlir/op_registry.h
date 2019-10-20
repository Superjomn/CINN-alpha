#pragma once

#include <memory>

#include "cinn/hlir/operator.h"

namespace cinn {
namespace hlir {

/**
 * Hold all the operators.
 */
class OpRegistry {
 public:
  using op_creator = std::function<std::unique_ptr<Operator>()>;

  /**
   * Singleton of OpRegistry.
   * @return global instance of OpRegistry.
   */
  static OpRegistry& Global() {
    static OpRegistry x;
    return x;
  }

  /**
   * Create a instance of Operator.
   * @param layer layer in HLIR.
   * @param type type of the operator.
   * @return an Operator instance.
   */
  std::unique_ptr<Operator> CreateOp(HlirLayer layer, const std::string& type);

  /**
   * Register a creator of the Operator.
   * @param layer layer in HLIR.
   * @param type type of the operator.
   * @param creator the lambda creator of this operator.
   */
  void RegistorOp(HlirLayer layer, const std::string& type, op_creator&& creator);

 private:
  OpRegistry() { registry_.resize(static_cast<int>(HlirLayer::__NUM__)); }

  std::vector<std::map<std::string, op_creator>> registry_;
};

/**
 * A helper class to make the registry of a operator creator easier.
 */
struct OpRegistor {
  OpRegistor(HlirLayer layer, const std::string& type, OpRegistry::op_creator&& creator) {
    OpRegistry::Global().RegistorOp(layer, type, std::move(creator));
  }

  int Touch() { return 0; }
};

}  // namespace hlir
}  // namespace cinn

#define REGISTER_OP(type__, layer__, class__) \
  ::cinn::hlir::OpRegistor type__##layer__(   \
      ::cinn::hlir::HlirLayer::layer__, #type__, [] { return std::unique_ptr<::cinn::hlir::Operator>(new class__); });

#define USE_OP(type__, layer__)                    \
  extern ::cinn::hlir::OpRegistor type__##layer__; \
  int type__##layer__##_use_var __attribute__((unused)) = type__##layer__.Touch();
