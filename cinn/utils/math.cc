#include "cinn/utils/math.h"

namespace cinn {

void RandomVec(float *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    data[i] = static_cast<float>(std::rand()) / (RAND_MAX + 1u) - 0.5f;
  }
}

}  // namespace cinn
