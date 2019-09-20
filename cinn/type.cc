#include "cinn/type.h"
#include <glog/logging.h>

namespace cinn {

int primitive_bytes(primitive_t type) {
  switch (type) {
    case primitive_t::uint8:
    case primitive_t::int8:
      return 1;
    case primitive_t::int16:
    case primitive_t::uint16:
      return 2;
    case primitive_t::int32:
    case primitive_t::uint32:
      return 4;
    case primitive_t::int64:
    case primitive_t::uint64:
      return 8;
    case primitive_t::boolean:
      return 1;
    default:
      LOG(FATAL) << "unsupported type " << static_cast<int>(type);
  }
}

}  // namespace cinn
