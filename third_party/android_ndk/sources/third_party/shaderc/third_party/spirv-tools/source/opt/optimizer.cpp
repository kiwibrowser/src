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

#include "spirv-tools/optimizer.hpp"

#include "build_module.h"
#include "make_unique.h"
#include "pass_manager.h"
#include "passes.h"

namespace spvtools {

struct Optimizer::PassToken::Impl {
  Impl(std::unique_ptr<opt::Pass> p) : pass(std::move(p)) {}

  std::unique_ptr<opt::Pass> pass;  // Internal implementation pass.
};

Optimizer::PassToken::PassToken(
    std::unique_ptr<Optimizer::PassToken::Impl> impl)
    : impl_(std::move(impl)) {}
Optimizer::PassToken::PassToken(PassToken&& that)
    : impl_(std::move(that.impl_)) {}

Optimizer::PassToken& Optimizer::PassToken::operator=(PassToken&& that) {
  impl_ = std::move(that.impl_);
  return *this;
}

Optimizer::PassToken::~PassToken() {}

struct Optimizer::Impl {
  explicit Impl(spv_target_env env) : target_env(env), pass_manager() {}

  const spv_target_env target_env;  // Target environment.
  opt::PassManager pass_manager;    // Internal implementation pass manager.
};

Optimizer::Optimizer(spv_target_env env) : impl_(new Impl(env)) {}

Optimizer::~Optimizer() {}

void Optimizer::SetMessageConsumer(MessageConsumer c) {
  // All passes' message consumer needs to be updated.
  for (uint32_t i = 0; i < impl_->pass_manager.NumPasses(); ++i) {
    impl_->pass_manager.GetPass(i)->SetMessageConsumer(c);
  }
  impl_->pass_manager.SetMessageConsumer(std::move(c));
}

Optimizer& Optimizer::RegisterPass(PassToken&& p) {
  // Change to use the pass manager's consumer.
  p.impl_->pass->SetMessageConsumer(impl_->pass_manager.consumer());
  impl_->pass_manager.AddPass(std::move(p.impl_->pass));
  return *this;
}

bool Optimizer::Run(const uint32_t* original_binary,
                    const size_t original_binary_size,
                    std::vector<uint32_t>* optimized_binary) const {
  std::unique_ptr<ir::Module> module =
      BuildModule(impl_->target_env, impl_->pass_manager.consumer(),
                  original_binary, original_binary_size);
  if (module == nullptr) return false;

  auto status = impl_->pass_manager.Run(module.get());
  if (status == opt::Pass::Status::SuccessWithChange ||
      (status == opt::Pass::Status::SuccessWithoutChange &&
       (optimized_binary->data() != original_binary ||
        optimized_binary->size() != original_binary_size))) {
    optimized_binary->clear();
    module->ToBinary(optimized_binary, /* skip_nop = */ true);
  }

  return status != opt::Pass::Status::Failure;
}

Optimizer::PassToken CreateNullPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(MakeUnique<opt::NullPass>());
}

Optimizer::PassToken CreateStripDebugInfoPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::StripDebugInfoPass>());
}

Optimizer::PassToken CreateSetSpecConstantDefaultValuePass(
    const std::unordered_map<uint32_t, std::string>& id_value_map) {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::SetSpecConstantDefaultValuePass>(id_value_map));
}

Optimizer::PassToken CreateSetSpecConstantDefaultValuePass(
    const std::unordered_map<uint32_t, std::vector<uint32_t>>& id_value_map) {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::SetSpecConstantDefaultValuePass>(id_value_map));
}

Optimizer::PassToken CreateFlattenDecorationPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::FlattenDecorationPass>());
}

Optimizer::PassToken CreateFreezeSpecConstantValuePass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::FreezeSpecConstantValuePass>());
}

Optimizer::PassToken CreateFoldSpecConstantOpAndCompositePass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::FoldSpecConstantOpAndCompositePass>());
}

Optimizer::PassToken CreateUnifyConstantPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::UnifyConstantPass>());
}

Optimizer::PassToken CreateEliminateDeadConstantPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::EliminateDeadConstantPass>());
}

Optimizer::PassToken CreateBlockMergePass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::BlockMergePass>());
}

Optimizer::PassToken CreateInlineExhaustivePass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::InlineExhaustivePass>());
}
  
Optimizer::PassToken CreateInlineOpaquePass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::InlineOpaquePass>());
}
  
Optimizer::PassToken CreateLocalAccessChainConvertPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::LocalAccessChainConvertPass>());
}
  
Optimizer::PassToken CreateLocalSingleBlockLoadStoreElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::LocalSingleBlockLoadStoreElimPass>());
}

Optimizer::PassToken CreateLocalSingleStoreElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::LocalSingleStoreElimPass>());
}

Optimizer::PassToken CreateInsertExtractElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::InsertExtractElimPass>());
}

Optimizer::PassToken CreateDeadBranchElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::DeadBranchElimPass>());
}

Optimizer::PassToken CreateLocalMultiStoreElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::LocalMultiStoreElimPass>());
}

Optimizer::PassToken CreateAggressiveDCEPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::AggressiveDCEPass>());
}

Optimizer::PassToken CreateCommonUniformElimPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::CommonUniformElimPass>());
}

Optimizer::PassToken CreateCompactIdsPass() {
  return MakeUnique<Optimizer::PassToken::Impl>(
      MakeUnique<opt::CompactIdsPass>());
}

}  // namespace spvtools
