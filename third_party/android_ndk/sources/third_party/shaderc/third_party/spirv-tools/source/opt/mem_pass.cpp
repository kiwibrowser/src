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

#include "mem_pass.h"

#include "iterator.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kStorePtrIdInIdx = 0;
const uint32_t kLoadPtrIdInIdx = 0;
const uint32_t kAccessChainPtrIdInIdx = 0;
const uint32_t kCopyObjectOperandInIdx = 0;
const uint32_t kTypePointerStorageClassInIdx = 0;
const uint32_t kTypePointerTypeIdInIdx = 1;

}  // namespace anonymous


bool MemPass::IsBaseTargetType(
    const ir::Instruction* typeInst) const {
  switch (typeInst->opcode()) {
  case SpvOpTypeInt:
  case SpvOpTypeFloat:
  case SpvOpTypeBool:
  case SpvOpTypeVector:
  case SpvOpTypeMatrix:
  case SpvOpTypeImage:
  case SpvOpTypeSampler:
  case SpvOpTypeSampledImage:
    return true;
  default:
    break;
  }
  return false;
}

bool MemPass::IsTargetType(
    const ir::Instruction* typeInst) const {
  if (IsBaseTargetType(typeInst))
    return true;
  if (typeInst->opcode() == SpvOpTypeArray)
    return IsBaseTargetType(
        def_use_mgr_->GetDef(typeInst->GetSingleWordOperand(1)));
  if (typeInst->opcode() != SpvOpTypeStruct)
    return false;
  // All struct members must be math type
  int nonMathComp = 0;
  typeInst->ForEachInId([&nonMathComp,this](const uint32_t* tid) {
    ir::Instruction* compTypeInst = def_use_mgr_->GetDef(*tid);
    if (!IsBaseTargetType(compTypeInst)) ++nonMathComp;
  });
  return nonMathComp == 0;
}

bool MemPass::IsNonPtrAccessChain(const SpvOp opcode) const {
  return opcode == SpvOpAccessChain || opcode == SpvOpInBoundsAccessChain;
}

bool MemPass::IsPtr(uint32_t ptrId) {
  uint32_t varId = ptrId;
  ir::Instruction* ptrInst = def_use_mgr_->GetDef(varId);
  while (ptrInst->opcode() == SpvOpCopyObject) {
    varId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = def_use_mgr_->GetDef(varId);
  }
  const SpvOp op = ptrInst->opcode();
  if (op == SpvOpVariable || IsNonPtrAccessChain(op))
    return true;
  if (op != SpvOpFunctionParameter)
    return false;
  const uint32_t varTypeId = ptrInst->type_id();
  const ir::Instruction* varTypeInst = def_use_mgr_->GetDef(varTypeId);
  return varTypeInst->opcode() == SpvOpTypePointer;
}

ir::Instruction* MemPass::GetPtr(
      uint32_t ptrId, uint32_t* varId) {
  *varId = ptrId;
  ir::Instruction* ptrInst = def_use_mgr_->GetDef(*varId);
  while (ptrInst->opcode() == SpvOpCopyObject) {
    *varId = ptrInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    ptrInst = def_use_mgr_->GetDef(*varId);
  }
  ir::Instruction* varInst = ptrInst;
  while (varInst->opcode() != SpvOpVariable && 
      varInst->opcode() != SpvOpFunctionParameter) {
    if (IsNonPtrAccessChain(varInst->opcode())) {
      *varId = varInst->GetSingleWordInOperand(kAccessChainPtrIdInIdx);
    }
    else {
      assert(varInst->opcode() == SpvOpCopyObject);
      *varId = varInst->GetSingleWordInOperand(kCopyObjectOperandInIdx);
    }
    varInst = def_use_mgr_->GetDef(*varId);
  }
  return ptrInst;
}

ir::Instruction* MemPass::GetPtr(
      ir::Instruction* ip, uint32_t* varId) {
  const SpvOp op = ip->opcode();
  assert(op == SpvOpStore || op == SpvOpLoad);
  const uint32_t ptrId = ip->GetSingleWordInOperand(
      op == SpvOpStore ? kStorePtrIdInIdx : kLoadPtrIdInIdx);
  return GetPtr(ptrId, varId);
}

bool MemPass::IsTargetVar(uint32_t varId) {
  if (seen_non_target_vars_.find(varId) != seen_non_target_vars_.end())
    return false;
  if (seen_target_vars_.find(varId) != seen_target_vars_.end())
    return true;
  const ir::Instruction* varInst = def_use_mgr_->GetDef(varId);
  if (varInst->opcode() != SpvOpVariable)
    return false;;
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = def_use_mgr_->GetDef(varTypeId);
  if (varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) !=
    SpvStorageClassFunction) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  const uint32_t varPteTypeId =
    varTypeInst->GetSingleWordInOperand(kTypePointerTypeIdInIdx);
  ir::Instruction* varPteTypeInst = def_use_mgr_->GetDef(varPteTypeId);
  if (!IsTargetType(varPteTypeInst)) {
    seen_non_target_vars_.insert(varId);
    return false;
  }
  seen_target_vars_.insert(varId);
  return true;
}

void MemPass::FindNamedOrDecoratedIds() {
  named_or_decorated_ids_.clear();
  for (auto& di : module_->debugs())
    if (di.opcode() == SpvOpName)
      named_or_decorated_ids_.insert(di.GetSingleWordInOperand(0));
  for (auto& ai : module_->annotations())
    if (ai.opcode() == SpvOpDecorate || ai.opcode() == SpvOpDecorateId)
      named_or_decorated_ids_.insert(ai.GetSingleWordInOperand(0));
}

bool MemPass::HasOnlyNamesAndDecorates(uint32_t id) const {
  analysis::UseList* uses = def_use_mgr_->GetUses(id);
  if (uses == nullptr)
    return true;
  if (named_or_decorated_ids_.find(id) == named_or_decorated_ids_.end())
    return false;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (op != SpvOpName && !IsNonTypeDecorate(op))
      return false;
  }
  return true;
}

void MemPass::KillNamesAndDecorates(uint32_t id) {
  // TODO(greg-lunarg): Remove id from any OpGroupDecorate and 
  // kill if no other operands.
  if (named_or_decorated_ids_.find(id) == named_or_decorated_ids_.end())
    return;
  analysis::UseList* uses = def_use_mgr_->GetUses(id);
  if (uses == nullptr)
    return;
  std::list<ir::Instruction*> killList;
  for (auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    if (op == SpvOpName || IsNonTypeDecorate(op))
      killList.push_back(u.inst);
  }
  for (auto kip : killList)
    def_use_mgr_->KillInst(kip);
}

void MemPass::KillNamesAndDecorates(ir::Instruction* inst) {
  const uint32_t rId = inst->result_id();
  if (rId == 0)
    return;
  KillNamesAndDecorates(rId);
}

bool MemPass::HasLoads(uint32_t varId) const {
  analysis::UseList* uses = def_use_mgr_->GetUses(varId);
  if (uses == nullptr)
    return false;
  for (auto u : *uses) {
    SpvOp op = u.inst->opcode();
    // TODO(): The following is slightly conservative. Could be
    // better handling of non-store/name.
    if (IsNonPtrAccessChain(op) || op == SpvOpCopyObject) {
      if (HasLoads(u.inst->result_id()))
        return true;
    }
    else if (op != SpvOpStore && op != SpvOpName)
      return true;
  }
  return false;
}

bool MemPass::IsLiveVar(uint32_t varId) const {
  const ir::Instruction* varInst = def_use_mgr_->GetDef(varId);
  // assume live if not a variable eg. function parameter
  if (varInst->opcode() != SpvOpVariable)
    return true;
  // non-function scope vars are live
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = def_use_mgr_->GetDef(varTypeId);
  if (varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) !=
      SpvStorageClassFunction)
    return true;
  // test if variable is loaded from
  return HasLoads(varId);
}

bool MemPass::IsLiveStore(ir::Instruction* storeInst) {
  // get store's variable
  uint32_t varId;
  (void) GetPtr(storeInst, &varId);
  return IsLiveVar(varId);
}

void MemPass::AddStores(
    uint32_t ptr_id, std::queue<ir::Instruction*>* insts) {
  analysis::UseList* uses = def_use_mgr_->GetUses(ptr_id);
  if (uses != nullptr) {
    for (auto u : *uses) {
      if (IsNonPtrAccessChain(u.inst->opcode()))
        AddStores(u.inst->result_id(), insts);
      else if (u.inst->opcode() == SpvOpStore)
        insts->push(u.inst);
    }
  }
}

void MemPass::DCEInst(ir::Instruction* inst) {
  std::queue<ir::Instruction*> deadInsts;
  deadInsts.push(inst);
  while (!deadInsts.empty()) {
    ir::Instruction* di = deadInsts.front();
    // Don't delete labels
    if (di->opcode() == SpvOpLabel) {
      deadInsts.pop();
      continue;
    }
    // Remember operands
    std::vector<uint32_t> ids;
    di->ForEachInId([&ids](uint32_t* iid) {
      ids.push_back(*iid);
    });
    uint32_t varId = 0;
    // Remember variable if dead load
    if (di->opcode() == SpvOpLoad)
      (void) GetPtr(di, &varId);
    KillNamesAndDecorates(di);
    def_use_mgr_->KillInst(di);
    // For all operands with no remaining uses, add their instruction
    // to the dead instruction queue.
    for (auto id : ids)
      if (HasOnlyNamesAndDecorates(id))
        deadInsts.push(def_use_mgr_->GetDef(id));
    // if a load was deleted and it was the variable's
    // last load, add all its stores to dead queue
    if (varId != 0 && !IsLiveVar(varId))
      AddStores(varId, &deadInsts);
    deadInsts.pop();
  }
}

void MemPass::ReplaceAndDeleteLoad(
    ir::Instruction* loadInst, uint32_t replId) {
  const uint32_t loadId = loadInst->result_id();
  KillNamesAndDecorates(loadId);
  (void) def_use_mgr_->ReplaceAllUsesWith(loadId, replId);
  DCEInst(loadInst);
}

MemPass::MemPass() : module_(nullptr), def_use_mgr_(nullptr) {}

}  // namespace opt
}  // namespace spvtools

