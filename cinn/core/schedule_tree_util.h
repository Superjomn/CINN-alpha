#pragma once

#include <memory>
#include <vector>
#include "cinn/core/schedule_tree.h"

/**
 * This file contains some utility methods for ScheduleTree.
 */

namespace cinn {

schedule_tree_ptr_t CreateScheduleTreeFromIsl(isl::schedule schedule);

}  // namespace cinn
