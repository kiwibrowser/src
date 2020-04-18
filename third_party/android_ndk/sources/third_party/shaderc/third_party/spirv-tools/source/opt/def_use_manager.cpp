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

#include "def_use_manager.h"

#include "log.h"
#include "reflect.h"

namespace spvtools {
namespace opt {
namespace analysis {

void DefUseManager::AnalyzeInstDef(ir::Instruction* inst) {
  const uint32_t def_id = inst->result_id();
  if (def_id != 0) {
    auto iter = id_to_def_.find(def_id);
    if (iter != id_to_def_.end()) {
      // Clear the original instruction that defining the same result id of the
      // new instruction.
      ClearInst(iter->second);
    }
    id_to_def_[def_id] = inst;
  }
  else {
    ClearInst(inst);
  }
}

void DefUseManager::AnalyzeInstUse(ir::Instruction* inst) {
  // Create entry for the given instruction. Note that the instruction may
  // not have any in-operands. In such cases, we still need a entry for those
  // instructions so this manager knows it has seen the instruction later.
  inst_to_used_ids_[inst] = {};

  for (uint32_t i = 0; i < inst->NumOperands(); ++i) {
    switch (inst->GetOperand(i).type) {
      // For any id type but result id type
    case SPV_OPERAND_TYPE_ID:
    case SPV_OPERAND_TYPE_TYPE_ID:
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID:
    case SPV_OPERAND_TYPE_SCOPE_ID: {
      uint32_t use_id = inst->GetSingleWordOperand(i);
      // use_id is used by the instruction generating def_id.
      id_to_uses_[use_id].push_back({ inst, i });
      inst_to_used_ids_[inst].push_back(use_id);
    } break;
    default:
      break;
    }
  }
}

void DefUseManager::AnalyzeInstDefUse(ir::Instruction* inst) {
  AnalyzeInstDef(inst);
  AnalyzeInstUse(inst);
}

ir::Instruction* DefUseManager::GetDef(uint32_t id) {
  auto iter = id_to_def_.find(id);
  if (iter == id_to_def_.end()) return nullptr;
  return iter->second;
}

UseList* DefUseManager::GetUses(uint32_t id) {
  auto iter = id_to_uses_.find(id);
  if (iter == id_to_uses_.end()) return nullptr;
  return &iter->second;
}

const UseList* DefUseManager::GetUses(uint32_t id) const {
  const auto iter = id_to_uses_.find(id);
  if (iter == id_to_uses_.end()) return nullptr;
  return &iter->second;
}

std::vector<ir::Instruction*> DefUseManager::GetAnnotations(uint32_t id) const {
  std::vector<ir::Instruction*> annos;
  const auto* uses = GetUses(id);
  if (!uses) return annos;
  for (const auto& c : *uses) {
    if (ir::IsAnnotationInst(c.inst->opcode())) {
      annos.push_back(c.inst);
    }
  }
  return annos;
}

bool DefUseManager::KillDef(uint32_t id) {
  auto iter = id_to_def_.find(id);
  if (iter == id_to_def_.end()) return false;
  KillInst(iter->second);
  return true;
}

void DefUseManager::KillInst(ir::Instruction* inst) {
  if (!inst) return;
  ClearInst(inst);
  inst->ToNop();
}

bool DefUseManager::ReplaceAllUsesWith(uint32_t before, uint32_t after) {
  if (before == after) return false;
  if (id_to_uses_.count(before) == 0) return false;

  for (auto it = id_to_uses_[before].cbegin(); it != id_to_uses_[before].cend();
       ++it) {
    const uint32_t type_result_id_count =
        (it->inst->result_id() != 0) + (it->inst->type_id() != 0);

    if (it->operand_index < type_result_id_count) {
      // Update the type_id. Note that result id is immutable so it should
      // never be updated.
      if (it->inst->type_id() != 0 && it->operand_index == 0) {
        it->inst->SetResultType(after);
      } else if (it->inst->type_id() == 0) {
        SPIRV_ASSERT(consumer_, false,
                     "Result type id considered as use while the instruction "
                     "doesn't have a result type id.");
        (void)consumer_;  // Makes the compiler happy for release build.
      } else {
        SPIRV_ASSERT(consumer_, false,
                     "Trying setting the immutable result id.");
      }
    } else {
      // Update an in-operand.
      uint32_t in_operand_pos = it->operand_index - type_result_id_count;
      // Make the modification in the instruction.
      it->inst->SetInOperand(in_operand_pos, {after});
    }
    // Update inst to used ids map
    auto iter = inst_to_used_ids_.find(it->inst);
    if (iter != inst_to_used_ids_.end())
      for (auto uit = iter->second.begin(); uit != iter->second.end(); uit++)
        if (*uit == before) *uit = after;
    // Register the use of |after| id into id_to_uses_.
    // TODO(antiagainst): de-duplication.
    id_to_uses_[after].push_back({it->inst, it->operand_index});
  }
  id_to_uses_.erase(before);
  return true;
}

void DefUseManager::AnalyzeDefUse(ir::Module* module) {
  if (!module) return;
  module->ForEachInst(std::bind(&DefUseManager::AnalyzeInstDefUse, this,
                                std::placeholders::_1));
}

void DefUseManager::ClearInst(ir::Instruction* inst) {
  auto iter = inst_to_used_ids_.find(inst);
  if (iter != inst_to_used_ids_.end()) {
    EraseUseRecordsOfOperandIds(inst);
    if (inst->result_id() != 0) {
      id_to_uses_.erase(inst->result_id());  // Remove all uses of this id.
      id_to_def_.erase(inst->result_id());
    }
  }
}

void DefUseManager::EraseUseRecordsOfOperandIds(const ir::Instruction* inst) {
  // Go through all ids used by this instruction, remove this instruction's
  // uses of them.
  auto iter = inst_to_used_ids_.find(inst);
  if (iter != inst_to_used_ids_.end()) {
    for (const auto use_id : iter->second) {
      auto uses_iter = id_to_uses_.find(use_id);
      if (uses_iter == id_to_uses_.end()) continue;
      auto& uses = uses_iter->second;
      for (auto it = uses.begin(); it != uses.end();) {
        if (it->inst == inst) {
          it = uses.erase(it);
        } else {
          ++it;
        }
      }
      if (uses.empty()) id_to_uses_.erase(use_id);
    }
    inst_to_used_ids_.erase(inst);
  }
}

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools
