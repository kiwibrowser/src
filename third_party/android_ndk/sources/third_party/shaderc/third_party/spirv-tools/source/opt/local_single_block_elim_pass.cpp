// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#include "local_single_block_elim_pass.h"

#include "iterator.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kStoreValIdInIdx = 1;

} // anonymous namespace

bool LocalSingleBlockLoadStoreElimPass::HasOnlySupportedRefs(uint32_t ptrId) {
  if (supported_ref_ptrs_.find(ptrId) != supported_ref_ptrs_.end())
    return true;
  analysis::UseList* uses = def_use_mgr_->GetUses(ptrId);
  assert(uses != nullptr);
  for (auto u : *uses) {
    SpvOp op = u.inst->opcode();
    if (IsNonPtrAccessChain(op) || op == SpvOpCopyObject) {
      if (!HasOnlySupportedRefs(u.inst->result_id())) return false;
    } else if (op != SpvOpStore && op != SpvOpLoad && op != SpvOpName &&
               !IsNonTypeDecorate(op))
      return false;
  }
  supported_ref_ptrs_.insert(ptrId);
  return true;
}

bool LocalSingleBlockLoadStoreElimPass::LocalSingleBlockLoadStoreElim(
    ir::Function* func) {
  // Perform local store/load and load/load elimination on each block
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    var2store_.clear();
    var2load_.clear();
    pinned_vars_.clear();
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      switch (ii->opcode()) {
      case SpvOpStore: {
        // Verify store variable is target type
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (!IsTargetVar(varId))
          continue;
        if (!HasOnlySupportedRefs(varId))
          continue;
        // Register the store
        if (ptrInst->opcode() == SpvOpVariable) {
          // if not pinned, look for WAW
          if (pinned_vars_.find(varId) == pinned_vars_.end()) {
            auto si = var2store_.find(varId);
            if (si != var2store_.end()) {
              def_use_mgr_->KillInst(si->second);
            }
          }
          var2store_[varId] = &*ii;
        }
        else {
          assert(IsNonPtrAccessChain(ptrInst->opcode()));
          var2store_.erase(varId);
        }
        pinned_vars_.erase(varId);
        var2load_.erase(varId);
      } break;
      case SpvOpLoad: {
        // Verify store variable is target type
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (!IsTargetVar(varId))
          continue;
        if (!HasOnlySupportedRefs(varId))
          continue;
        // Look for previous store or load
        uint32_t replId = 0;
        if (ptrInst->opcode() == SpvOpVariable) {
          auto si = var2store_.find(varId);
          if (si != var2store_.end()) {
            replId = si->second->GetSingleWordInOperand(kStoreValIdInIdx);
          }
          else {
            auto li = var2load_.find(varId);
            if (li != var2load_.end()) {
              replId = li->second->result_id();
            }
          }
        }
        if (replId != 0) {
          // replace load's result id and delete load
          ReplaceAndDeleteLoad(&*ii, replId);
          modified = true;
        }
        else {
          if (ptrInst->opcode() == SpvOpVariable)
            var2load_[varId] = &*ii;  // register load
          pinned_vars_.insert(varId);
        }
      } break;
      case SpvOpFunctionCall: {
        // Conservatively assume all locals are redefined for now.
        // TODO(): Handle more optimally
        var2store_.clear();
        var2load_.clear();
        pinned_vars_.clear();
      } break;
      default:
        break;
      }
    }
    // Go back and delete useless stores in block
    // TODO(greg-lunarg): Consider moving DCE into separate pass
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      if (ii->opcode() != SpvOpStore)
        continue;
      if (IsLiveStore(&*ii))
        continue;
      DCEInst(&*ii);
    }
  }
  return modified;
}

void LocalSingleBlockLoadStoreElimPass::Initialize(ir::Module* module) {

  module_ = module;

  // Initialize Target Type Caches
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();

  // Clear collections
  supported_ref_ptrs_.clear();

  // TODO(greg-lunarg): Reuse def/use from previous passes
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Start new ids with next availablein module
  next_id_ = module_->id_bound();

  // Initialize extensions whitelist
  InitExtensions();
};

bool LocalSingleBlockLoadStoreElimPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status LocalSingleBlockLoadStoreElimPass::ProcessImpl() {
  // Assumes logical addressing only
  if (module_->HasCapability(SpvCapabilityAddresses))
    return Status::SuccessWithoutChange;
  // Do not process if module contains OpGroupDecorate. Additional
  // support required in KillNamesAndDecorates().
  // TODO(greg-lunarg): Add support for OpGroupDecorate
  for (auto& ai : module_->annotations())
    if (ai.opcode() == SpvOpGroupDecorate)
      return Status::SuccessWithoutChange;
  // If any extensions in the module are not explicitly supported,
  // return unmodified. 
  if (!AllExtensionsSupported())
    return Status::SuccessWithoutChange;
  // Collect all named and decorated ids
  FindNamedOrDecoratedIds();
  // Process all entry point functions
  ProcessFunction pfn = [this](ir::Function* fp) {
    return LocalSingleBlockLoadStoreElim(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  FinalizeNextId(module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

LocalSingleBlockLoadStoreElimPass::LocalSingleBlockLoadStoreElimPass()
    : next_id_(0) {}

Pass::Status LocalSingleBlockLoadStoreElimPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void LocalSingleBlockLoadStoreElimPass::InitExtensions() {
  extensions_whitelist_.clear();
  extensions_whitelist_.insert({
    "SPV_AMD_shader_explicit_vertex_parameter",
    "SPV_AMD_shader_trinary_minmax",
    "SPV_AMD_gcn_shader",
    "SPV_KHR_shader_ballot",
    "SPV_AMD_shader_ballot",
    "SPV_AMD_gpu_shader_half_float",
    "SPV_KHR_shader_draw_parameters",
    "SPV_KHR_subgroup_vote",
    "SPV_KHR_16bit_storage",
    "SPV_KHR_device_group",
    "SPV_KHR_multiview",
    "SPV_NVX_multiview_per_view_attributes",
    "SPV_NV_viewport_array2",
    "SPV_NV_stereo_view_rendering",
    "SPV_NV_sample_mask_override_coverage",
    "SPV_NV_geometry_shader_passthrough",
    "SPV_AMD_texture_gather_bias_lod",
    "SPV_KHR_storage_buffer_storage_class",
    // SPV_KHR_variable_pointers
    //   Currently do not support extended pointer expressions
    "SPV_AMD_gpu_shader_int16",
    "SPV_KHR_post_depth_coverage",
    "SPV_KHR_shader_atomic_counter_ops",
  });
}

}  // namespace opt
}  // namespace spvtools
