// Copyright (c) 2016 Google Inc.
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

#include "pass_manager.h"

namespace spvtools {
namespace opt {

Pass::Status PassManager::Run(ir::Module* module) {
  auto status = Pass::Status::SuccessWithoutChange;
  for (const auto& pass : passes_) {
    const auto one_status = pass->Process(module);
    if (one_status == Pass::Status::Failure) return one_status;
    if (one_status == Pass::Status::SuccessWithChange) status = one_status;
  }
  // Set the Id bound in the header in case a pass forgot to do so.
  if (status == Pass::Status::SuccessWithChange) {
    module->SetIdBound(module->ComputeIdBound());
  }
  return status;
}

}  // namespace opt
}  // namespace spvtools
