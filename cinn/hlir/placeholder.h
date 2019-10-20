#pragma once
#include "cinn/hlir/tensor.h"
namespace cinn {
namespace hlir {

/**
 * PlaceHolder is the data source of a model.
 */
class Placeholder : public Tensor {
 public:
};

}  // namespace hlir
}  // namespace cinn
