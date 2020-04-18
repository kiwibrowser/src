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

#include "local_ssa_elim_pass.h"

#include "iterator.h"
#include "cfa.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kStoreValIdInIdx = 1;
const uint32_t kTypePointerTypeIdInIdx = 1;
const uint32_t kSelectionMergeMergeBlockIdInIdx = 0;
const uint32_t kLoopMergeMergeBlockIdInIdx = 0;
const uint32_t kLoopMergeContinueBlockIdInIdx = 1;

} // anonymous namespace

bool LocalMultiStoreElimPass::HasOnlySupportedRefs(uint32_t varId) {
  if (supported_ref_vars_.find(varId) != supported_ref_vars_.end())
    return true;
  analysis::UseList* uses = def_use_mgr_->GetUses(varId);
  if (uses == nullptr)
    return true;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (op != SpvOpStore && op != SpvOpLoad && op != SpvOpName &&
        !IsNonTypeDecorate(op))
      return false;
  }
  supported_ref_vars_.insert(varId);
  return true;
}

void LocalMultiStoreElimPass::InitSSARewrite(ir::Function& func) {
  // Init predecessors
  label2preds_.clear();
  for (auto& blk : func) {
    uint32_t blkId = blk.id();
    blk.ForEachSuccessorLabel([&blkId, this](uint32_t sbid) {
      label2preds_[sbid].push_back(blkId);
    });
  }
  // Collect target (and non-) variable sets. Remove variables with
  // non-load/store refs from target variable set
  for (auto& blk : func) {
    for (auto& inst : blk) {
      switch (inst.opcode()) {
        case SpvOpStore:
        case SpvOpLoad: {
          uint32_t varId;
          (void) GetPtr(&inst, &varId);
          if (!IsTargetVar(varId))
            break;
          if (HasOnlySupportedRefs(varId))
            break;
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
        } break;
        default:
          break;
      }
    }
  }
}

uint32_t LocalMultiStoreElimPass::MergeBlockIdIfAny(const ir::BasicBlock& blk,
    uint32_t* cbid) {
  auto merge_ii = blk.cend();
  --merge_ii;
  *cbid = 0;
  uint32_t mbid = 0;
  if (merge_ii != blk.cbegin()) {
    --merge_ii;
    if (merge_ii->opcode() == SpvOpLoopMerge) {
      mbid = merge_ii->GetSingleWordInOperand(kLoopMergeMergeBlockIdInIdx);
      *cbid = merge_ii->GetSingleWordInOperand(kLoopMergeContinueBlockIdInIdx);
    }
    else if (merge_ii->opcode() == SpvOpSelectionMerge) {
      mbid = merge_ii->GetSingleWordInOperand(kSelectionMergeMergeBlockIdInIdx);
    }
  }
  return mbid;
}

void LocalMultiStoreElimPass::ComputeStructuredSuccessors(ir::Function* func) {
  for (auto& blk : *func) {
    // If no predecessors in function, make successor to pseudo entry
    if (label2preds_[blk.id()].size() == 0)
      block2structured_succs_[&pseudo_entry_block_].push_back(&blk);
    // If header, make merge block first successor.
    uint32_t cbid;
    const uint32_t mbid = MergeBlockIdIfAny(blk, &cbid);
    if (mbid != 0) {
      block2structured_succs_[&blk].push_back(id2block_[mbid]);
      if (cbid != 0)
        block2structured_succs_[&blk].push_back(id2block_[cbid]);
    }
    // add true successors
    blk.ForEachSuccessorLabel([&blk, this](uint32_t sbid) {
      block2structured_succs_[&blk].push_back(id2block_[sbid]);
    });
  }
}

void LocalMultiStoreElimPass::ComputeStructuredOrder(
    ir::Function* func, std::list<ir::BasicBlock*>* order) {
  // Compute structured successors and do DFS
  ComputeStructuredSuccessors(func);
  auto ignore_block = [](cbb_ptr) {};
  auto ignore_edge = [](cbb_ptr, cbb_ptr) {};
  auto get_structured_successors = [this](const ir::BasicBlock* block) {
      return &(block2structured_succs_[block]); };
  // TODO(greg-lunarg): Get rid of const_cast by making moving const
  // out of the cfa.h prototypes and into the invoking code.
  auto post_order = [&](cbb_ptr b) {
      order->push_front(const_cast<ir::BasicBlock*>(b)); };
  
  spvtools::CFA<ir::BasicBlock>::DepthFirstTraversal(
      &pseudo_entry_block_, get_structured_successors, ignore_block,
      post_order, ignore_edge);
}

void LocalMultiStoreElimPass::SSABlockInitSinglePred(ir::BasicBlock* block_ptr) {
  // Copy map entry from single predecessor
  const uint32_t label = block_ptr->id();
  const uint32_t predLabel = label2preds_[label].front();
  assert(visitedBlocks_.find(predLabel) != visitedBlocks_.end());
  label2ssa_map_[label] = label2ssa_map_[predLabel];
}

bool LocalMultiStoreElimPass::IsLiveAfter(uint32_t var_id, uint32_t label) const {
  // For now, return very conservative result: true. This will result in
  // correct, but possibly usused, phi code to be generated. A subsequent
  // DCE pass should eliminate this code.
  // TODO(greg-lunarg): Return more accurate information
  (void) var_id;
  (void) label;
  return true;
}

uint32_t LocalMultiStoreElimPass::Type2Undef(uint32_t type_id) {
  const auto uitr = type2undefs_.find(type_id);
  if (uitr != type2undefs_.end())
    return uitr->second;
  const uint32_t undefId = TakeNextId();
  std::unique_ptr<ir::Instruction> undef_inst(
    new ir::Instruction(SpvOpUndef, type_id, undefId, {}));
  def_use_mgr_->AnalyzeInstDefUse(&*undef_inst);
  module_->AddGlobalValue(std::move(undef_inst));
  type2undefs_[type_id] = undefId;
  return undefId;
}

uint32_t LocalMultiStoreElimPass::GetPointeeTypeId(
    const ir::Instruction* ptrInst) const {
  const uint32_t ptrTypeId = ptrInst->type_id();
  const ir::Instruction* ptrTypeInst = def_use_mgr_->GetDef(ptrTypeId);
  return ptrTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
}

void LocalMultiStoreElimPass::SSABlockInitLoopHeader(
    std::list<ir::BasicBlock*>::iterator block_itr) {
  const uint32_t label = (*block_itr)->id();
  // Determine backedge label.
  uint32_t backLabel = 0;
  for (uint32_t predLabel : label2preds_[label])
    if (visitedBlocks_.find(predLabel) == visitedBlocks_.end()) {
      assert(backLabel == 0);
      backLabel = predLabel;
      break;
    }
  assert(backLabel != 0);
  // Determine merge block.
  auto mergeInst = (*block_itr)->end();
  --mergeInst;
  --mergeInst;
  uint32_t mergeLabel = mergeInst->GetSingleWordInOperand(
      kLoopMergeMergeBlockIdInIdx);
  // Collect all live variables and a default value for each across all
  // non-backedge predecesors. Must be ordered map because phis are
  // generated based on order and test results will otherwise vary across
  // platforms.
  std::map<uint32_t, uint32_t> liveVars;
  for (uint32_t predLabel : label2preds_[label]) {
    for (auto var_val : label2ssa_map_[predLabel]) {
      uint32_t varId = var_val.first;
      liveVars[varId] = var_val.second;
    }
  }
  // Add all stored variables in loop. Set their default value id to zero.
  for (auto bi = block_itr; (*bi)->id() != mergeLabel; ++bi) {
    ir::BasicBlock* bp = *bi;
    for (auto ii = bp->begin(); ii != bp->end(); ++ii) {
      if (ii->opcode() != SpvOpStore)
        continue;
      uint32_t varId;
      (void) GetPtr(&*ii, &varId);
      if (!IsTargetVar(varId))
        continue;
      liveVars[varId] = 0;
    }
  }
  // Insert phi for all live variables that require them. All variables
  // defined in loop require a phi. Otherwise all variables
  // with differing predecessor values require a phi.
  auto insertItr = (*block_itr)->begin();
  for (auto var_val : liveVars) {
    const uint32_t varId = var_val.first;
    if (!IsLiveAfter(varId, label))
      continue;
    const uint32_t val0Id = var_val.second;
    bool needsPhi = false;
    if (val0Id != 0) {
      for (uint32_t predLabel : label2preds_[label]) {
        // Skip back edge predecessor.
        if (predLabel == backLabel)
          continue;
        const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
        // Missing (undef) values always cause difference with (defined) value
        if (var_val_itr == label2ssa_map_[predLabel].end()) {
          needsPhi = true;
          break;
        }
        if (var_val_itr->second != val0Id) {
          needsPhi = true;
          break;
        }
      }
    }
    else {
      needsPhi = true;
    }
    // If val is the same for all predecessors, enter it in map
    if (!needsPhi) {
      label2ssa_map_[label].insert(var_val);
      continue;
    }
    // Val differs across predecessors. Add phi op to block and 
    // add its result id to the map. For back edge predecessor,
    // use the variable id. We will patch this after visiting back
    // edge predecessor. For predecessors that do not define a value,
    // use undef.
    std::vector<ir::Operand> phi_in_operands;
    uint32_t typeId = GetPointeeTypeId(def_use_mgr_->GetDef(varId));
    for (uint32_t predLabel : label2preds_[label]) {
      uint32_t valId;
      if (predLabel == backLabel) {
        valId = varId;
      }
      else {
        const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
        if (var_val_itr == label2ssa_map_[predLabel].end())
          valId = Type2Undef(typeId);
        else
          valId = var_val_itr->second;
      }
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {valId}});
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {predLabel}});
    }
    const uint32_t phiId = TakeNextId();
    std::unique_ptr<ir::Instruction> newPhi(
      new ir::Instruction(SpvOpPhi, typeId, phiId, phi_in_operands));
    // Only analyze the phi define now; analyze the phi uses after the
    // phi backedge predecessor value is patched.
    def_use_mgr_->AnalyzeInstDef(&*newPhi);
    insertItr = insertItr.InsertBefore(std::move(newPhi));
    ++insertItr;
    label2ssa_map_[label].insert({ varId, phiId });
  }
}

void LocalMultiStoreElimPass::SSABlockInitMultiPred(ir::BasicBlock* block_ptr) {
  const uint32_t label = block_ptr->id();
  // Collect all live variables and a default value for each across all
  // predecesors. Must be ordered map because phis are generated based on
  // order and test results will otherwise vary across platforms.
  std::map<uint32_t, uint32_t> liveVars;
  for (uint32_t predLabel : label2preds_[label]) {
    assert(visitedBlocks_.find(predLabel) != visitedBlocks_.end());
    for (auto var_val : label2ssa_map_[predLabel]) {
      const uint32_t varId = var_val.first;
      liveVars[varId] = var_val.second;
    }
  }
  // For each live variable, look for a difference in values across
  // predecessors that would require a phi and insert one.
  auto insertItr = block_ptr->begin();
  for (auto var_val : liveVars) {
    const uint32_t varId = var_val.first;
    if (!IsLiveAfter(varId, label))
      continue;
    const uint32_t val0Id = var_val.second;
    bool differs = false;
    for (uint32_t predLabel : label2preds_[label]) {
      const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
      // Missing values cause a difference because we'll need to create an
      // undef for that predecessor.
      if (var_val_itr == label2ssa_map_[predLabel].end()) {
        differs = true;
        break;
      }
      if (var_val_itr->second != val0Id) {
        differs = true;
        break;
      }
    }
    // If val is the same for all predecessors, enter it in map
    if (!differs) {
      label2ssa_map_[label].insert(var_val);
      continue;
    }
    // Val differs across predecessors. Add phi op to block and 
    // add its result id to the map
    std::vector<ir::Operand> phi_in_operands;
    const uint32_t typeId = GetPointeeTypeId(def_use_mgr_->GetDef(varId));
    for (uint32_t predLabel : label2preds_[label]) {
      const auto var_val_itr = label2ssa_map_[predLabel].find(varId);
      // If variable not defined on this path, use undef
      const uint32_t valId = (var_val_itr != label2ssa_map_[predLabel].end()) ?
          var_val_itr->second : Type2Undef(typeId);
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {valId}});
      phi_in_operands.push_back(
          {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {predLabel}});
    }
    const uint32_t phiId = TakeNextId();
    std::unique_ptr<ir::Instruction> newPhi(
      new ir::Instruction(SpvOpPhi, typeId, phiId, phi_in_operands));
    def_use_mgr_->AnalyzeInstDefUse(&*newPhi);
    insertItr = insertItr.InsertBefore(std::move(newPhi));
    ++insertItr;
    label2ssa_map_[label].insert({varId, phiId});
  }
}

bool LocalMultiStoreElimPass::IsLoopHeader(ir::BasicBlock* block_ptr) const {
  auto iItr = block_ptr->end();
  --iItr;
  if (iItr == block_ptr->begin())
    return false;
  --iItr;
  return iItr->opcode() == SpvOpLoopMerge;
}

void LocalMultiStoreElimPass::SSABlockInit(
    std::list<ir::BasicBlock*>::iterator block_itr) {
  const size_t numPreds = label2preds_[(*block_itr)->id()].size();
  if (numPreds == 0)
    return;
  if (numPreds == 1)
    SSABlockInitSinglePred(*block_itr);
  else if (IsLoopHeader(*block_itr))
    SSABlockInitLoopHeader(block_itr);
  else
    SSABlockInitMultiPred(*block_itr);
}

void LocalMultiStoreElimPass::PatchPhis(uint32_t header_id, uint32_t back_id) {
  ir::BasicBlock* header = id2block_[header_id];
  auto phiItr = header->begin();
  for (; phiItr->opcode() == SpvOpPhi; ++phiItr) {
    uint32_t cnt = 0;
    uint32_t idx;
    phiItr->ForEachInId([&cnt,&back_id,&idx](uint32_t* iid) {
      if (cnt % 2 == 1 && *iid == back_id) idx = cnt - 1;
      ++cnt;
    });
    // Use undef if variable not in backedge predecessor map
    const uint32_t varId = phiItr->GetSingleWordInOperand(idx);
    const auto valItr = label2ssa_map_[back_id].find(varId);
    uint32_t valId = (valItr != label2ssa_map_[back_id].end()) ?
      valItr->second :
      Type2Undef(GetPointeeTypeId(def_use_mgr_->GetDef(varId)));
    phiItr->SetInOperand(idx, { valId });
    // Analyze uses now that they are complete
    def_use_mgr_->AnalyzeInstUse(&*phiItr);
  }
}

bool LocalMultiStoreElimPass::EliminateMultiStoreLocal(ir::Function* func) {
  InitSSARewrite(*func);
  // Process all blocks in structured order. This is just one way (the
  // simplest?) to make sure all predecessors blocks are processed before
  // a block itself.
  std::list<ir::BasicBlock*> structuredOrder;
  ComputeStructuredOrder(func, &structuredOrder);
  bool modified = false;
  for (auto bi = structuredOrder.begin(); bi != structuredOrder.end(); ++bi) {
    // Skip pseudo entry block
    if (*bi == &pseudo_entry_block_)
      continue;
    // Initialize this block's label2ssa_map_ entry using predecessor maps.
    // Then process all stores and loads of targeted variables.
    SSABlockInit(bi);
    ir::BasicBlock* bp = *bi;
    const uint32_t label = bp->id();
    for (auto ii = bp->begin(); ii != bp->end(); ++ii) {
      switch (ii->opcode()) {
        case SpvOpStore: {
          uint32_t varId;
          (void) GetPtr(&*ii, &varId);
          if (!IsTargetVar(varId))
            break;
          // Register new stored value for the variable
          label2ssa_map_[label][varId] =
              ii->GetSingleWordInOperand(kStoreValIdInIdx);
        } break;
        case SpvOpLoad: {
          uint32_t varId;
          (void) GetPtr(&*ii, &varId);
          if (!IsTargetVar(varId))
            break;
          uint32_t replId = 0;
          const auto ssaItr = label2ssa_map_.find(label);
          if (ssaItr != label2ssa_map_.end()) {
            const auto valItr = ssaItr->second.find(varId);
            if (valItr != ssaItr->second.end())
              replId = valItr->second;
          }
          // If variable is not defined, use undef
          if (replId == 0) {
            replId = Type2Undef(GetPointeeTypeId(def_use_mgr_->GetDef(varId)));
          }
          // Replace load's id with the last stored value id for variable
          // and delete load. Kill any names or decorates using id before
          // replacing to prevent incorrect replacement in those instructions.
          const uint32_t loadId = ii->result_id();
          KillNamesAndDecorates(loadId);
          (void)def_use_mgr_->ReplaceAllUsesWith(loadId, replId);
          def_use_mgr_->KillInst(&*ii);
          modified = true;
        } break;
        default: {
        } break;
      }
    }
    visitedBlocks_.insert(label);
    // Look for successor backedge and patch phis in loop header
    // if found.
    uint32_t header = 0;
    bp->ForEachSuccessorLabel([&header,this](uint32_t succ) {
      if (visitedBlocks_.find(succ) == visitedBlocks_.end()) return;
      assert(header == 0);
      header = succ;
    });
    if (header != 0)
      PatchPhis(header, label);
  }
  // Remove all target variable stores.
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      if (ii->opcode() != SpvOpStore)
        continue;
      uint32_t varId;
      (void) GetPtr(&*ii, &varId);
      if (!IsTargetVar(varId))
        continue;
      assert(!HasLoads(varId));
      DCEInst(&*ii);
      modified = true;
    }
  }
  return modified;
}

void LocalMultiStoreElimPass::Initialize(ir::Module* module) {

  module_ = module;

  // TODO(greg-lunarg): Reuse def/use from previous passes
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize function and block maps
  id2block_.clear();
  block2structured_succs_.clear();
  for (auto& fn : *module_)
    for (auto& blk : fn)
      id2block_[blk.id()] = &blk;

  // Clear collections
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();
  visitedBlocks_.clear();
  type2undefs_.clear();
  supported_ref_vars_.clear();
  block2structured_succs_.clear();
  label2preds_.clear();
  label2ssa_map_.clear();

  // Start new ids with next availablein module
  next_id_ = module_->id_bound();

  // Initialize extension whitelist
  InitExtensions();
};

bool LocalMultiStoreElimPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status LocalMultiStoreElimPass::ProcessImpl() {
  // Assumes all control flow structured.
  // TODO(greg-lunarg): Do SSA rewrite for non-structured control flow
  if (!module_->HasCapability(SpvCapabilityShader))
    return Status::SuccessWithoutChange;
  // Assumes logical addressing only
  // TODO(greg-lunarg): Add support for physical addressing
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
  // Process functions
  ProcessFunction pfn = [this](ir::Function* fp) {
    return EliminateMultiStoreLocal(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  FinalizeNextId(module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

LocalMultiStoreElimPass::LocalMultiStoreElimPass()
    : pseudo_entry_block_(std::unique_ptr<ir::Instruction>(
          new ir::Instruction(SpvOpLabel, 0, 0, {}))),
      next_id_(0) {}

Pass::Status LocalMultiStoreElimPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void LocalMultiStoreElimPass::InitExtensions() {
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

