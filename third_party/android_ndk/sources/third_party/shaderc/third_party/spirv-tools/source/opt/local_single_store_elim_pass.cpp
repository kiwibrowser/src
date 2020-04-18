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

#include "local_single_store_elim_pass.h"

#include "cfa.h"
#include "iterator.h"
#include "spirv/1.0/GLSL.std.450.h"

// Universal Limit of ResultID + 1
static const int kInvalidId = 0x400000;

namespace spvtools {
namespace opt {

namespace {

const uint32_t kStoreValIdInIdx = 1;

} // anonymous namespace

bool LocalSingleStoreElimPass::HasOnlySupportedRefs(uint32_t ptrId) {
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

void LocalSingleStoreElimPass::SingleStoreAnalyze(ir::Function* func) {
  ssa_var2store_.clear();
  non_ssa_vars_.clear();
  store2idx_.clear();
  store2blk_.clear();
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    uint32_t instIdx = 0;
    for (auto ii = bi->begin(); ii != bi->end(); ++ii, ++instIdx) {
      switch (ii->opcode()) {
      case SpvOpStore: {
        // Verify store variable is target type
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (non_ssa_vars_.find(varId) != non_ssa_vars_.end())
          continue;
        if (ptrInst->opcode() != SpvOpVariable) {
          non_ssa_vars_.insert(varId);
          ssa_var2store_.erase(varId);
          continue;
        }
        // Verify target type and function storage class
        if (!IsTargetVar(varId)) {
          non_ssa_vars_.insert(varId);
          continue;
        }
        if (!HasOnlySupportedRefs(varId)) {
          non_ssa_vars_.insert(varId);
          continue;
        }
        // Ignore variables with multiple stores
        if (ssa_var2store_.find(varId) != ssa_var2store_.end()) {
          non_ssa_vars_.insert(varId);
          ssa_var2store_.erase(varId);
          continue;
        }
        // Remember pointer to variable's store and it's
        // ordinal position in block
        ssa_var2store_[varId] = &*ii;
        store2idx_[&*ii] = instIdx;
        store2blk_[&*ii] = &*bi;
      } break;
      default:
        break;
      } // switch
    }
  }
}

LocalSingleStoreElimPass::GetBlocksFunction
LocalSingleStoreElimPass::AugmentedCFGSuccessorsFunction() const {
  return [this](const ir::BasicBlock* block) {
    auto asmi = augmented_successors_map_.find(block);
    if (asmi != augmented_successors_map_.end())
      return &(*asmi).second;
    auto smi = successors_map_.find(block);
    return &(*smi).second;
  };
}

LocalSingleStoreElimPass::GetBlocksFunction
LocalSingleStoreElimPass::AugmentedCFGPredecessorsFunction() const {
  return [this](const ir::BasicBlock* block) {
    auto apmi = augmented_predecessors_map_.find(block);
    if (apmi != augmented_predecessors_map_.end())
      return &(*apmi).second;
    auto pmi = predecessors_map_.find(block);
    return &(*pmi).second;
  };
}

void LocalSingleStoreElimPass::CalculateImmediateDominators(
    ir::Function* func) {
  // Compute CFG
  vector<ir::BasicBlock*> ordered_blocks;
  predecessors_map_.clear();
  successors_map_.clear();
  for (auto& blk : *func) {
    ordered_blocks.push_back(&blk);
    blk.ForEachSuccessorLabel([&blk, &ordered_blocks, this](uint32_t sbid) {
      successors_map_[&blk].push_back(label2block_[sbid]);
      predecessors_map_[label2block_[sbid]].push_back(&blk);
    });
  }
  // Compute Augmented CFG
  augmented_successors_map_.clear();
  augmented_predecessors_map_.clear();
  successors_map_[&pseudo_exit_block_] = {};
  predecessors_map_[&pseudo_entry_block_] = {};
  auto succ_func = [this](const ir::BasicBlock* b)
    { return &successors_map_[b]; };
  auto pred_func = [this](const ir::BasicBlock* b)
    { return &predecessors_map_[b]; };
  CFA<ir::BasicBlock>::ComputeAugmentedCFG(
    ordered_blocks,
    &pseudo_entry_block_,
    &pseudo_exit_block_,
    &augmented_successors_map_,
    &augmented_predecessors_map_,
    succ_func,
    pred_func);
  // Compute Dominators
  vector<const ir::BasicBlock*> postorder;
  auto ignore_block = [](cbb_ptr) {};
  auto ignore_edge = [](cbb_ptr, cbb_ptr) {};
  spvtools::CFA<ir::BasicBlock>::DepthFirstTraversal(
    ordered_blocks[0], AugmentedCFGSuccessorsFunction(),
    ignore_block, [&](cbb_ptr b) { postorder.push_back(b); },
    ignore_edge);
  auto edges = spvtools::CFA<ir::BasicBlock>::CalculateDominators(
    postorder, AugmentedCFGPredecessorsFunction());
  idom_.clear();
  for (auto edge : edges)
    idom_[edge.first] = edge.second;
}

bool LocalSingleStoreElimPass::Dominates(
    ir::BasicBlock* blk0, uint32_t idx0,
    ir::BasicBlock* blk1, uint32_t idx1) {
  if (blk0 == blk1)
    return idx0 <= idx1;
  ir::BasicBlock* b = blk1;
  while (idom_[b] != b) {
    b = idom_[b];
    if (b == blk0)
      return true;
  }
  return false;
}

bool LocalSingleStoreElimPass::SingleStoreProcess(ir::Function* func) {
  CalculateImmediateDominators(func);
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    uint32_t instIdx = 0;
    for (auto ii = bi->begin(); ii != bi->end(); ++ii, ++instIdx) {
      if (ii->opcode() != SpvOpLoad)
        continue;
      uint32_t varId;
      ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
      // Skip access chain loads
      if (ptrInst->opcode() != SpvOpVariable)
        continue;
      const auto vsi = ssa_var2store_.find(varId);
      if (vsi == ssa_var2store_.end())
        continue;
      if (non_ssa_vars_.find(varId) != non_ssa_vars_.end())
        continue;
      // store must dominate load
      if (!Dominates(store2blk_[vsi->second], store2idx_[vsi->second], &*bi, instIdx))
        continue;
      // Use store value as replacement id
      uint32_t replId = vsi->second->GetSingleWordInOperand(kStoreValIdInIdx);
      // replace all instances of the load's id with the SSA value's id
      ReplaceAndDeleteLoad(&*ii, replId);
      modified = true;
    }
  }
  return modified;
}

bool LocalSingleStoreElimPass::SingleStoreDCE() {
  bool modified = false;
  for (auto v : ssa_var2store_) {
    // check that it hasn't already been DCE'd
    if (v.second->opcode() != SpvOpStore)
      continue;
    if (non_ssa_vars_.find(v.first) != non_ssa_vars_.end())
      continue;
    if (!IsLiveStore(v.second)) {
      DCEInst(v.second);
      modified = true;
    }
  }
  return modified;
}

bool LocalSingleStoreElimPass::LocalSingleStoreElim(ir::Function* func) {
  bool modified = false;
  SingleStoreAnalyze(func);
  if (ssa_var2store_.empty())
    return false;
  modified |= SingleStoreProcess(func);
  modified |= SingleStoreDCE();
  return modified;
}

void LocalSingleStoreElimPass::Initialize(ir::Module* module) {
  module_ = module;

  // Initialize function and block maps
  label2block_.clear();
  for (auto& fn : *module_) {
    for (auto& blk : fn) {
      uint32_t bid = blk.id();
      label2block_[bid] = &blk;
    }
  }

  // Initialize Target Type Caches
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();

  // Initialize Supported Ref Pointer Cache
  supported_ref_ptrs_.clear();

  // TODO: Reuse def/use (and other state) from previous passes
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize next unused Id
  next_id_ = module_->id_bound();

  // Initialize extension whitelist
  InitExtensions();
};

bool LocalSingleStoreElimPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status LocalSingleStoreElimPass::ProcessImpl() {
  // Assumes logical addressing only
  if (module_->HasCapability(SpvCapabilityAddresses))
    return Status::SuccessWithoutChange;
  // Do not process if module contains OpGroupDecorate. Additional
  // support required in KillNamesAndDecorates().
  // TODO(greg-lunarg): Add support for OpGroupDecorate
  for (auto& ai : module_->annotations())
    if (ai.opcode() == SpvOpGroupDecorate)
      return Status::SuccessWithoutChange;
  // Do not process if any disallowed extensions are enabled
  if (!AllExtensionsSupported())
    return Status::SuccessWithoutChange;
  // Collect all named and decorated ids
  FindNamedOrDecoratedIds();
  // Process all entry point functions
  ProcessFunction pfn = [this](ir::Function* fp) {
    return LocalSingleStoreElim(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  FinalizeNextId(module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

LocalSingleStoreElimPass::LocalSingleStoreElimPass()
    : pseudo_entry_block_(std::unique_ptr<ir::Instruction>(
          new ir::Instruction(SpvOpLabel, 0, 0, {}))),
      pseudo_exit_block_(std::unique_ptr<ir::Instruction>(
          new ir::Instruction(SpvOpLabel, 0, kInvalidId, {}))),
      next_id_(0) {}

Pass::Status LocalSingleStoreElimPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void LocalSingleStoreElimPass::InitExtensions() {
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
