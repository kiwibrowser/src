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

#include "local_access_chain_convert_pass.h"

#include "iterator.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kStoreValIdInIdx = 1;
const uint32_t kAccessChainPtrIdInIdx = 0;
const uint32_t kTypePointerTypeIdInIdx = 1;
const uint32_t kConstantValueInIdx = 0;
const uint32_t kTypeIntWidthInIdx = 0;

} // anonymous namespace

void LocalAccessChainConvertPass::DeleteIfUseless(ir::Instruction* inst) {
  const uint32_t resId = inst->result_id();
  assert(resId != 0);
  if (HasOnlyNamesAndDecorates(resId)) {
    KillNamesAndDecorates(resId);
    def_use_mgr_->KillInst(inst);
  }
}

uint32_t LocalAccessChainConvertPass::GetPointeeTypeId(
    const ir::Instruction* ptrInst) const {
  const uint32_t ptrTypeId = ptrInst->type_id();
  const ir::Instruction* ptrTypeInst = def_use_mgr_->GetDef(ptrTypeId);
  return ptrTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
}

void LocalAccessChainConvertPass::BuildAndAppendInst(
    SpvOp opcode,
    uint32_t typeId,
    uint32_t resultId,
    const std::vector<ir::Operand>& in_opnds,
    std::vector<std::unique_ptr<ir::Instruction>>* newInsts) {
  std::unique_ptr<ir::Instruction> newInst(new ir::Instruction(
      opcode, typeId, resultId, in_opnds));
  def_use_mgr_->AnalyzeInstDefUse(&*newInst);
  newInsts->emplace_back(std::move(newInst));
}

uint32_t LocalAccessChainConvertPass::BuildAndAppendVarLoad(
    const ir::Instruction* ptrInst,
    uint32_t* varId,
    uint32_t* varPteTypeId,
    std::vector<std::unique_ptr<ir::Instruction>>* newInsts) {
  const uint32_t ldResultId = TakeNextId();
  *varId = ptrInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx);
  const ir::Instruction* varInst = def_use_mgr_->GetDef(*varId);
  assert(varInst->opcode() == SpvOpVariable);
  *varPteTypeId = GetPointeeTypeId(varInst);
  BuildAndAppendInst(SpvOpLoad, *varPteTypeId, ldResultId,
      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {*varId}}}, newInsts);
  return ldResultId;
}

void LocalAccessChainConvertPass::AppendConstantOperands(
    const ir::Instruction* ptrInst,
    std::vector<ir::Operand>* in_opnds) {
  uint32_t iidIdx = 0;
  ptrInst->ForEachInId([&iidIdx, &in_opnds, this](const uint32_t *iid) {
    if (iidIdx > 0) {
      const ir::Instruction* cInst = def_use_mgr_->GetDef(*iid);
      uint32_t val = cInst->GetSingleWordInOperand(kConstantValueInIdx);
      in_opnds->push_back(
        {spv_operand_type_t::SPV_OPERAND_TYPE_LITERAL_INTEGER, {val}});
    }
    ++iidIdx;
  });
}

uint32_t LocalAccessChainConvertPass::GenAccessChainLoadReplacement(
    const ir::Instruction* ptrInst,
    std::vector<std::unique_ptr<ir::Instruction>>* newInsts) {

  // Build and append load of variable in ptrInst
  uint32_t varId;
  uint32_t varPteTypeId;
  const uint32_t ldResultId = BuildAndAppendVarLoad(ptrInst, &varId,
                                                    &varPteTypeId, newInsts);

  // Build and append Extract
  const uint32_t extResultId = TakeNextId();
  const uint32_t ptrPteTypeId = GetPointeeTypeId(ptrInst);
  std::vector<ir::Operand> ext_in_opnds = 
      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {ldResultId}}};
  AppendConstantOperands(ptrInst, &ext_in_opnds);
  BuildAndAppendInst(SpvOpCompositeExtract, ptrPteTypeId, extResultId, 
                     ext_in_opnds, newInsts);
  return extResultId;
}

void LocalAccessChainConvertPass::GenAccessChainStoreReplacement(
    const ir::Instruction* ptrInst,
    uint32_t valId,
    std::vector<std::unique_ptr<ir::Instruction>>* newInsts) {

  // Build and append load of variable in ptrInst
  uint32_t varId;
  uint32_t varPteTypeId;
  const uint32_t ldResultId = BuildAndAppendVarLoad(ptrInst, &varId,
                                                    &varPteTypeId, newInsts);

  // Build and append Insert
  const uint32_t insResultId = TakeNextId();
  std::vector<ir::Operand> ins_in_opnds = 
      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {valId}}, 
       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {ldResultId}}};
  AppendConstantOperands(ptrInst, &ins_in_opnds);
  BuildAndAppendInst(
      SpvOpCompositeInsert, varPteTypeId, insResultId, ins_in_opnds, newInsts);

  // Build and append Store
  BuildAndAppendInst(SpvOpStore, 0, 0, 
      {{spv_operand_type_t::SPV_OPERAND_TYPE_ID, {varId}}, 
       {spv_operand_type_t::SPV_OPERAND_TYPE_ID, {insResultId}}},
      newInsts);
}

bool LocalAccessChainConvertPass::IsConstantIndexAccessChain(
    const ir::Instruction* acp) const {
  uint32_t inIdx = 0;
  uint32_t nonConstCnt = 0;
  acp->ForEachInId([&inIdx, &nonConstCnt, this](const uint32_t* tid) {
    if (inIdx > 0) {
      ir::Instruction* opInst = def_use_mgr_->GetDef(*tid);
      if (opInst->opcode() != SpvOpConstant) ++nonConstCnt;
    }
    ++inIdx;
  });
  return nonConstCnt == 0;
}

bool LocalAccessChainConvertPass::HasOnlySupportedRefs(uint32_t ptrId) {
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

void LocalAccessChainConvertPass::FindTargetVars(ir::Function* func) {
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      switch (ii->opcode()) {
      case SpvOpStore:
      case SpvOpLoad: {
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (!IsTargetVar(varId))
          break;
        const SpvOp op = ptrInst->opcode();
        // Rule out variables with non-supported refs eg function calls
        if (!HasOnlySupportedRefs(varId)) {
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
          break;
        }
        // Rule out variables with nested access chains
        // TODO(): Convert nested access chains
        if (IsNonPtrAccessChain(op) &&
            ptrInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx) != varId) {
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
          break;
        }
        // Rule out variables accessed with non-constant indices
        if (!IsConstantIndexAccessChain(ptrInst)) {
          seen_non_target_vars_.insert(varId);
          seen_target_vars_.erase(varId);
          break;
        }
      } break;
      default:
        break;
      }
    }
  }
}

bool LocalAccessChainConvertPass::ConvertLocalAccessChains(ir::Function* func) {
  FindTargetVars(func);
  // Replace access chains of all targeted variables with equivalent
  // extract and insert sequences
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ++bi) {
    for (auto ii = bi->begin(); ii != bi->end(); ++ii) {
      switch (ii->opcode()) {
      case SpvOpLoad: {
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (!IsNonPtrAccessChain(ptrInst->opcode()))
          break;
        if (!IsTargetVar(varId))
          break;
        std::vector<std::unique_ptr<ir::Instruction>> newInsts;
        uint32_t replId =
            GenAccessChainLoadReplacement(ptrInst, &newInsts);
        ReplaceAndDeleteLoad(&*ii, replId);
        ++ii;
        ii = ii.InsertBefore(&newInsts);
        ++ii;
        modified = true;
      } break;
      case SpvOpStore: {
        uint32_t varId;
        ir::Instruction* ptrInst = GetPtr(&*ii, &varId);
        if (!IsNonPtrAccessChain(ptrInst->opcode()))
          break;
        if (!IsTargetVar(varId))
          break;
        std::vector<std::unique_ptr<ir::Instruction>> newInsts;
        uint32_t valId = ii->GetSingleWordInOperand(kStoreValIdInIdx);
        GenAccessChainStoreReplacement(ptrInst, valId, &newInsts);
        def_use_mgr_->KillInst(&*ii);
        DeleteIfUseless(ptrInst);
        ++ii;
        ii = ii.InsertBefore(&newInsts);
        ++ii;
        ++ii;
        modified = true;
      } break;
      default:
        break;
      }
    }
  }
  return modified;
}

void LocalAccessChainConvertPass::Initialize(ir::Module* module) {

  module_ = module;

  // Initialize Target Variable Caches
  seen_target_vars_.clear();
  seen_non_target_vars_.clear();

  // Initialize collections
  supported_ref_ptrs_.clear();

  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize next unused Id.
  next_id_ = module->id_bound();

  // Initialize extension whitelist
  InitExtensions();
};

bool LocalAccessChainConvertPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status LocalAccessChainConvertPass::ProcessImpl() {
  // If non-32-bit integer type in module, terminate processing
  // TODO(): Handle non-32-bit integer constants in access chains
  for (const ir::Instruction& inst : module_->types_values())
    if (inst.opcode() == SpvOpTypeInt &&
        inst.GetSingleWordInOperand(kTypeIntWidthInIdx) != 32)
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
  // Process all entry point functions.
  ProcessFunction pfn = [this](ir::Function* fp) {
    return ConvertLocalAccessChains(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  FinalizeNextId(module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

LocalAccessChainConvertPass::LocalAccessChainConvertPass() : next_id_(0) {}

Pass::Status LocalAccessChainConvertPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void LocalAccessChainConvertPass::InitExtensions() {
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

