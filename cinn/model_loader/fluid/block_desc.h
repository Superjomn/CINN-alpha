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

#pragma once

#include <glog/logging.h>
#include <vector>
#include "cinn/model_loader/fluid/framework.pb.h"

namespace cinn {
namespace model_loader {
namespace fluid {

using namespace paddle;  // NOLINT

class BlockDesc {
 public:
  BlockDesc() = delete;

  explicit BlockDesc(framework::proto::BlockDesc* desc) : desc_(desc) { CHECK(desc_); }

  framework::proto::BlockDesc* Proto() { return desc_; }

  const framework::proto::BlockDesc& ReadonlyProto() const { return *desc_; }

  int32_t Idx() const { return desc_->idx(); }

  void SetIdx(int32_t idx) { desc_->set_idx(idx); }

  int32_t ParentIdx() const { return desc_->parent_idx(); }

  void SetParentIdx(int32_t idx) { desc_->set_parent_idx(idx); }

  size_t VarsSize() const { return desc_->vars_size(); }

  void ClearVars() { desc_->clear_vars(); }

  template <typename T>
  T* GetVar(int32_t idx);

  template <typename T>
  T* AddVar();

  size_t OpsSize() const { return desc_->ops_size(); }

  void ClearOps() { desc_->clear_ops(); }

  template <typename T>
  T* GetOp(int32_t idx);

  template <typename T>
  T* AddOp();

  int32_t ForwardBlockIdx() const { return desc_->forward_block_idx(); }

  void SetForwardBlockIdx(int32_t idx) { desc_->set_forward_block_idx(idx); }

 private:
  framework::proto::BlockDesc* desc_;  // not_own
};

}  // namespace fluid
}  // namespace model_loader
}  // namespace cinn
