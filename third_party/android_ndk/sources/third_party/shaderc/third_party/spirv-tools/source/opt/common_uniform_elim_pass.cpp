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

#include "common_uniform_elim_pass.h"

#include "cfa.h"
#include "iterator.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kAccessChainPtrIdInIdx = 0;
const uint32_t kTypePointerStorageClassInIdx = 0;
const uint32_t kTypePointerTypeIdInIdx = 1;
const uint32_t kConstantValueInIdx = 0;
const uint32_t kExtractCompositeIdInIdx = 0;
const uint32_t kExtractIdx0InIdx = 1;
const uint32_t kSelectionMergeMergeBlockIdInIdx = 0;
const uint32_t kLoopMergeMergeBlockIdInIdx = 0;
const uint32_t kLoopMergeContinueBlockIdInIdx = 1;
const uint32_t kStorePtrIdInIdx = 0;
const uint32_t kLoadPtrIdInIdx = 0;
const uint32_t kCopyObjectOperandInIdx = 0;
const uint32_t kTypeIntWidthInIdx = 0;

} // anonymous namespace

bool CommonUniformElimPass::IsNonPtrAccessChain(const SpvOp opcode) const {
  return opcode == SpvOpAccessChain || opcode == SpvOpInBoundsAccessChain;
}

bool CommonUniformElimPass::IsSamplerOrImageType(
  const ir::Instruction* typeInst) const {
  switch (typeInst->opcode()) {
    case SpvOpTypeSampler:
    case SpvOpTypeImage:
    case SpvOpTypeSampledImage:
      return true;
    default:
      break;
  }
  if (typeInst->opcode() != SpvOpTypeStruct)
    return false;
  // Return true if any member is a sampler or image
  int samplerOrImageCnt = 0;
  typeInst->ForEachInId([&samplerOrImageCnt, this](const uint32_t* tid) {
    const ir::Instruction* compTypeInst = def_use_mgr_->GetDef(*tid);
    if (IsSamplerOrImageType(compTypeInst)) ++samplerOrImageCnt;
  });
  return samplerOrImageCnt > 0;
}

bool CommonUniformElimPass::IsSamplerOrImageVar(
    uint32_t varId) const {
  const ir::Instruction* varInst = def_use_mgr_->GetDef(varId);
  assert(varInst->opcode() == SpvOpVariable);
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = def_use_mgr_->GetDef(varTypeId);
  const uint32_t varPteTypeId =
    varTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
  ir::Instruction* varPteTypeInst = def_use_mgr_->GetDef(varPteTypeId);
  return IsSamplerOrImageType(varPteTypeInst);
}

bool CommonUniformElimPass::IsLoopHeader(ir::BasicBlock* block_ptr) {
  auto iItr = block_ptr->tail();
  if (iItr == block_ptr->begin())
    return false;
  --iItr;
  return iItr->opcode() == SpvOpLoopMerge;
}

uint32_t CommonUniformElimPass::MergeBlockIdIfAny(const ir::BasicBlock& blk,
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

ir::Instruction* CommonUniformElimPass::GetPtr(
      ir::Instruction* ip, uint32_t* objId) {
  const SpvOp op = ip->opcode();
  assert(op == SpvOpStore || op == SpvOpLoad);
  *objId = ip->GetSingleWordInOperand(
      op == SpvOpStore ? kStorePtrIdInIdx : kLoadPtrIdInIdx);
  ir::Instruction* ptrInst = def_use_mgr_->GetDef(*objId);
  while (ptrInst->opcode() == SpvOpCopyObject) {
    *objId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = def_use_mgr_->GetDef(*objId);
  }
  ir::Instruction* objInst = ptrInst;
  while (objInst->opcode() != SpvOpVariable &&
      objInst->opcode() != SpvOpFunctionParameter) {
    if (IsNonPtrAccessChain(objInst->opcode())) {
      *objId = objInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx);
    }
    else {
      assert(objInst->opcode() == SpvOpCopyObject);
      *objId = objInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    }
    objInst = def_use_mgr_->GetDef(*objId);
  }
  return ptrInst;
}

bool CommonUniformElimPass::IsUniformVar(uint32_t varId) {
  const ir::Instruction* varInst =
    def_use_mgr_->id_to_defs().find(varId)->second;
  if (varInst->opcode() != SpvOpVariable)
    return false;
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst =
    def_use_mgr_->id_to_defs().find(varTypeId)->second;
  return varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) ==
    SpvStorageClassUniform ||
    varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) ==
    SpvStorageClassUniformConstant;
}

bool CommonUniformElimPass::HasUnsupportedDecorates(uint32_t id) const {
  analysis::UseList* uses = def_use_mgr_->GetUses(id);
  if (uses == nullptr)
    return false;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (IsNonTypeDecorate(op))
      return true;
  }
  return false;
}

bool CommonUniformElimPass::HasOnlyNamesAndDecorates(uint32_t id) const {
  analysis::UseList* uses = def_use_mgr_->GetUses(id);
  if (uses == nullptr)
    return true;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (op != SpvOpName && !IsNonTypeDecorate(op))
      return false;
  }
  return true;
}

void CommonUniformElimPass::KillNamesAndDecorates(uint32_t id) {
  // TODO(greg-lunarg): Remove id from any OpGroupDecorate and 
  // kill if no other operands.
  analysis::UseList* uses = def_use_mgr_->GetUses(id);
  if (uses == nullptr)
    return;
  std::list<ir::Instruction*> killList;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (op != SpvOpName && !IsNonTypeDecorate(op))
      continue;
    killList.push_back(u.inst);
  }
  for (auto kip : killList)
    def_use_mgr_->KillInst(kip);
}

void CommonUniformElimPass::KillNamesAndDecorates(ir::Instruction* inst) {
  // TODO(greg-lunarg): Remove inst from any OpGroupDecorate and 
  // kill if not other operands.
  const uint32_t rId = inst->result_id();
  if (rId == 0)
    return;
  KillNamesAndDecorates(rId);
}

void CommonUniformElimPass::DeleteIfUseless(ir::Instruction* inst) {
  const uint32_t resId = inst->result_id();
  assert(resId != 0);
  if (HasOnlyNamesAndDecorates(resId)) {
    KillNamesAndDecorates(resId);
    def_use_mgr_->KillInst(inst);
  }
}

void CommonUniformElimPass::ReplaceAndDeleteLoad(ir::Instruction* loadInst,
                                      uint32_t replId,
                                      ir::Instruction* ptrInst) {
  const uint32_t loadId = loadInst->result_id();
  KillNamesAndDecorates(loadId);
  (void) def_use_mgr_->ReplaceAllUsesWith(loadId, replId);
  // remove load instruction
  def_use_mgr_->KillInst(loadInst);
  // if access chain, see if it can be removed as well
  if (IsNonPtrAccessChain(ptrInst->opcode()))
    DeleteIfUseless(ptrInst);
}

uint32_t CommonUniformElimPass::GetPointeeTypeId(const ir::Instruction* ptrInst) {
  const uint32_t ptrTypeId = ptrInst->type_id();
  const ir::Instruction* ptrTypeInst = def_use_mgr_->GetDef(ptrTypeId);
  return ptrTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
}

void CommonUniformElimPass::GenACLoadRepl(const ir::Instruction* ptrInst,
  std::vector<std::unique_ptr<ir::Instruction>>* newInsts,
  uint32_t* resultId) {

  // Build and append Load
  const uint32_t ldResultId = TakeNextId();
  const uint32_t varId =
    ptrInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx);
  const ir::Instruction* varInst = def_use_mgr_->GetDef(varId);
  assert(varInst->opcode() == SpvOpVariable);
  const uint32_t varPteTypeId = GetPointeeTypeId(varInst);
  std::vector<ir::Operand> load_in_operands;
  load_in_operands.push_back(
    ir::Operand(spv_operand_type_t::SPV_OPERAND_TYPE_ID,
      std::initializer_list<uint32_t>{varId}));
  std::unique_ptr<ir::Instruction> newLoad(new ir::Instruction(SpvOpLoad,
    varPteTypeId, ldResultId, load_in_operands));
  def_use_mgr_->AnalyzeInstDefUse(&*newLoad);
  newInsts->emplace_back(std::move(newLoad));

  // Build and append Extract
  const uint32_t extResultId = TakeNextId();
  const uint32_t ptrPteTypeId = GetPointeeTypeId(ptrInst);
  std::vector<ir::Operand> ext_in_opnds;
  ext_in_opnds.push_back(
    ir::Operand(spv_operand_type_t::SPV_OPERAND_TYPE_ID,
      std::initializer_list<uint32_t>{ldResultId}));
  uint32_t iidIdx = 0;
  ptrInst->ForEachInId([&iidIdx, &ext_in_opnds, this](const uint32_t *iid) {
    if (iidIdx > 0) {
      const ir::Instruction* cInst = def_use_mgr_->GetDef(*iid);
      uint32_t val = cInst->GetSingleWordInOperand(kConstantValueInIdx);
      ext_in_opnds.push_back(
        ir::Operand(spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER,
          std::initializer_list<uint32_t>{val}));
    }
    ++iidIdx;
  });
  std::unique_ptr<ir::Instruction> newExt(new ir::Instruction(
    SpvOpCompositeExtract, ptrPteTypeId, extResultId, ext_in_opnds));
  def_use_mgr_->AnalyzeInstDefUse(&*newExt);
  newInsts->emplace_back(std::move(newExt));
  *resultId = extResultId;
}

bool CommonUniformElimPass::IsConstantIndexAccessChain(ir::Instruction* acp) {
  uint32_t inIdx = 0;
  uint32_t nonConstCnt = 0;
  acp->ForEachInId([&inIdx, &nonConstCnt, this](uint32_t* tid) {
    if (inIdx > 0) {
      ir::Instruction* opInst = def_use_mgr_->GetDef(*tid);
      if (opInst->opcode() != SpvOpConstant) ++nonConstCnt;
    }
    ++inIdx;
  });
  return nonConstCnt == 0;
}

bool CommonUniformElimPass::UniformAccessChainConvert(ir::Function* func) {
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      if (ii->opcode() != SpvOpLoad)
        continue;
      uint32_t varId;
      ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
      if (!IsNonPtrAccessChain(ptrInst->opcode()))
        continue;
      // Do not convert nested access chains
      if (ptrInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx) != varId)
        continue;
      if (!IsUniformVar(varId))
        continue;
      if (!IsConstantIndexAccessChain(ptrInst))
        continue;
      if (HasUnsupportedDecorates(ii->result_id()))
        continue;
      if (HasUnsupportedDecorates(ptrInst->result_id()))
        continue;
      std::vector<std::unique_ptr<ir::Instruction>> newInsts;
      uint32_t replId;
      GenACLoadRepl(ptrInst, &newInsts, &replId);
      ReplaceAndDeleteLoad(&*ii, replId, ptrInst);
      ++ii;
      ii = ii.InsertBefore(&newInsts);
      ++ii;
      modified = true;
    }
  }
  return modified;
}

void CommonUniformElimPass::ComputeStructuredSuccessors(ir::Function* func) {
  for (auto& blk : *func) {
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

void CommonUniformElimPass::ComputeStructuredOrder(
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
  
  order->clear();
  spvtools::CFA<ir::BasicBlock>::DepthFirstTraversal(
      &*func->begin(), get_structured_successors, ignore_block,
      post_order, ignore_edge);
}

bool CommonUniformElimPass::CommonUniformLoadElimination(ir::Function* func) {
  // Process all blocks in structured order. This is just one way (the
  // simplest?) to keep track of the most recent block outside of control
  // flow, used to copy common instructions, guaranteed to dominate all
  // following load sites.
  std::list<ir::BasicBlock*> structuredOrder;
  ComputeStructuredOrder(func, &structuredOrder);
  uniform2load_id_.clear();
  bool modified = false;
  // Find insertion point in first block to copy non-dominating loads.
  auto insertItr = func->begin()->begin();
  while (insertItr->opcode() == SpvOpVariable ||
      insertItr->opcode() == SpvOpNop)
    ++insertItr;
  uint32_t mergeBlockId = 0;
  for (auto bi = structuredOrder.begin(); bi != structuredOrder.end(); ++bi) {
    ir::BasicBlock* bp = *bi;
    // Check if we are exiting outermost control construct. If so, remember
    // new load insertion point. Trying to keep register pressure down.
    if (mergeBlockId == bp->id()) {
      mergeBlockId = 0;
      insertItr = bp->begin();
    }
    for (auto ii = bp->begin(); ii != bp->end(); ++ii) {
      if (ii->opcode() != SpvOpLoad)
        continue;
      uint32_t varId;
      ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
      if (ptrInst->opcode() != SpvOpVariable)
        continue;
      if (!IsUniformVar(varId))
        continue;
      if (IsSamplerOrImageVar(varId))
        continue;
      if (HasUnsupportedDecorates(ii->result_id()))
        continue;
      uint32_t replId;
      const auto uItr = uniform2load_id_.find(varId);
      if (uItr != uniform2load_id_.end()) {
        replId = uItr->second;
      }
      else {
        if (mergeBlockId == 0) {
          // Load is in dominating block; just remember it
          uniform2load_id_[varId] = ii->result_id();
          continue;
        }
        else {
          // Copy load into most recent dominating block and remember it
          replId = TakeNextId();
          std::unique_ptr<ir::Instruction> newLoad(new ir::Instruction(SpvOpLoad,
            ii->type_id(), replId, {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {varId}}}));
          def_use_mgr_->AnalyzeInstDefUse(&*newLoad);
          insertItr = insertItr.InsertBefore(std::move(newLoad));
          ++insertItr;
          uniform2load_id_[varId] = replId;
        }
      }
      ReplaceAndDeleteLoad(&*ii, replId, ptrInst);
      modified = true;
    }
    // If we are outside of any control construct and entering one, remember
    // the id of the merge block
    if (mergeBlockId == 0) {
      uint32_t dummy;
      mergeBlockId = MergeBlockIdIfAny(*bp, &dummy);
    }
  }
  return modified;
}

bool CommonUniformElimPass::CommonUniformLoadElimBlock(ir::Function* func) {
  bool modified = false;
  for (auto& blk : *func) {
    uniform2load_id_.clear();
    for (auto ii = blk.begin(); ii != blk.end(); ++ii) {
      if (ii->opcode() != SpvOpLoad)
        continue;
      uint32_t varId;
      ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
      if (ptrInst->opcode() != SpvOpVariable)
        continue;
      if (!IsUniformVar(varId))
        continue;
      if (!IsSamplerOrImageVar(varId))
        continue;
      if (HasUnsupportedDecorates(ii->result_id()))
        continue;
      uint32_t replId;
      const auto uItr = uniform2load_id_.find(varId);
      if (uItr != uniform2load_id_.end()) {
        replId = uItr->second;
      }
      else {
        uniform2load_id_[varId] = ii->result_id();
        continue;
      }
      ReplaceAndDeleteLoad(&*ii, replId, ptrInst);
      modified = true;
    }
  }
  return modified;
}

bool CommonUniformElimPass::CommonExtractElimination(ir::Function* func) {
  // Find all composite ids with duplicate extracts.
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      if (ii->opcode() != SpvOpCompositeExtract)
        continue;
      // TODO(greg-lunarg): Support multiple indices
      if (ii->NumInOperands() > 2)
        continue;
      if (HasUnsupportedDecorates(ii->result_id()))
        continue;
      uint32_t compId = ii->GetSingleWordInOperand(kExtractCompositeIdInIdx);
      uint32_t idx = ii->GetSingleWordInOperand(kExtractIdx0InIdx);
      comp2idx2inst_[compId][idx].push_back(&*ii);
    }
  }
  // For all defs of ids with duplicate extracts, insert new extracts
  // after def, and replace and delete old extracts
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      const auto cItr = comp2idx2inst_.find(ii->result_id());
      if (cItr == comp2idx2inst_.end())
        continue;
      for (auto idxItr : cItr->second) {
        if (idxItr.second.size() < 2)
          continue;
        uint32_t replId = TakeNextId();
        std::unique_ptr<ir::Instruction> newExtract(new ir::Instruction(*idxItr.second.front()));
        newExtract->SetResultId(replId);
        def_use_mgr_->AnalyzeInstDefUse(&*newExtract);
        ++ii;
        ii = ii.InsertBefore(std::move(newExtract));
        for (auto instItr : idxItr.second) {
          uint32_t resId = instItr->result_id();
          KillNamesAndDecorates(resId);
          (void)def_use_mgr_->ReplaceAllUsesWith(resId, replId);
          def_use_mgr_->KillInst(instItr);
        }
        modified = true;
      }
    }
  }
  return modified;
}

bool CommonUniformElimPass::EliminateCommonUniform(ir::Function* func) {
    bool modified = false;
    modified |= UniformAccessChainConvert(func);
    modified |= CommonUniformLoadElimination(func);
    modified |= CommonExtractElimination(func);

    modified |= CommonUniformLoadElimBlock(func);
    return modified;
}

void CommonUniformElimPass::Initialize(ir::Module* module) {

  module_ = module;

  // Initialize function and block maps
  id2block_.clear();
  for (auto& fn : *module_)
    for (auto& blk : fn)
      id2block_[blk.id()] = &blk;

  // Clear collections
  block2structured_succs_.clear();
  label2preds_.clear();
  comp2idx2inst_.clear();

  // TODO(greg-lunarg): Use def/use from previous pass
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize next unused Id.
  next_id_ = module->id_bound();

  // Initialize extension whitelist
  InitExtensions();
};


bool CommonUniformElimPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status CommonUniformElimPass::ProcessImpl() {
  // Assumes all control flow structured.
  // TODO(greg-lunarg): Do SSA rewrite for non-structured control flow
  if (!module_->HasCapability(SpvCapabilityShader))
    return Status::SuccessWithoutChange;
  // Assumes logical addressing only
  // TODO(greg-lunarg): Add support for physical addressing
  if (module_->HasCapability(SpvCapabilityAddresses))
    return Status::SuccessWithoutChange;
  // Do not process if any disallowed extensions are enabled
  if (!AllExtensionsSupported())
      return Status::SuccessWithoutChange;
  // Do not process if module contains OpGroupDecorate. Additional
  // support required in KillNamesAndDecorates().
  // TODO(greg-lunarg): Add support for OpGroupDecorate
  for (auto& ai : module_->annotations())
    if (ai.opcode() == SpvOpGroupDecorate)
      return Status::SuccessWithoutChange;
  // If non-32-bit integer type in module, terminate processing
  // TODO(): Handle non-32-bit integer constants in access chains
  for (const ir::Instruction& inst : module_->types_values())
    if (inst.opcode() == SpvOpTypeInt &&
        inst.GetSingleWordInOperand(kTypeIntWidthInIdx) != 32)
      return Status::SuccessWithoutChange;
  // Process entry point functions
  ProcessFunction pfn = [this](ir::Function* fp) {
    return EliminateCommonUniform(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  FinalizeNextId(module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

CommonUniformElimPass::CommonUniformElimPass()
    : module_(nullptr), def_use_mgr_(nullptr), next_id_(0) {}

Pass::Status CommonUniformElimPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void CommonUniformElimPass::InitExtensions() {
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

