// Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cinn/model_loader/fluid/program_desc.h"

namespace cinn {
namespace model_loader {
namespace fluid {

template <>
framework::proto::BlockDesc* ProgramDesc::GetBlock<framework::proto::BlockDesc>(int32_t idx) {
  CHECK_LT(idx, BlocksSize()) << "idx >= blocks.size()";
  return desc_->mutable_blocks(idx);
}

template <>
framework::proto::BlockDesc* ProgramDesc::AddBlock<framework::proto::BlockDesc>() {
  return desc_->add_blocks();
}

}  // namespace fluid
}  // namespace model_loader
}  // namespace cinn
