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

#include "module.h"

#include <algorithm>
#include <cstring>

#include "operand.h"
#include "reflect.h"

namespace spvtools {
namespace ir {

std::vector<Instruction*> Module::GetTypes() {
  std::vector<Instruction*> insts;
  for (uint32_t i = 0; i < types_values_.size(); ++i) {
    if (IsTypeInst(types_values_[i]->opcode()))
      insts.push_back(types_values_[i].get());
  }
  return insts;
};

std::vector<const Instruction*> Module::GetTypes() const {
  std::vector<const Instruction*> insts;
  for (uint32_t i = 0; i < types_values_.size(); ++i) {
    if (IsTypeInst(types_values_[i]->opcode()))
      insts.push_back(types_values_[i].get());
  }
  return insts;
};

std::vector<Instruction*> Module::GetConstants() {
  std::vector<Instruction*> insts;
  for (uint32_t i = 0; i < types_values_.size(); ++i) {
    if (IsConstantInst(types_values_[i]->opcode()))
      insts.push_back(types_values_[i].get());
  }
  return insts;
};

std::vector<const Instruction*> Module::GetConstants() const {
  std::vector<const Instruction*> insts;
  for (uint32_t i = 0; i < types_values_.size(); ++i) {
    if (IsConstantInst(types_values_[i]->opcode()))
      insts.push_back(types_values_[i].get());
  }
  return insts;
};

uint32_t Module::GetGlobalValue(SpvOp opcode) const {
  for (uint32_t i = 0; i < types_values_.size(); ++i) {
    if (types_values_[i]->opcode() == opcode)
      return types_values_[i]->result_id();
  }
  return 0;
}

void Module::AddGlobalValue(SpvOp opcode, uint32_t result_id,
                            uint32_t type_id) {
  std::unique_ptr<ir::Instruction> newGlobal(
      new ir::Instruction(opcode, type_id, result_id, {}));
  AddGlobalValue(std::move(newGlobal));
}

void Module::ForEachInst(const std::function<void(Instruction*)>& f,
                         bool run_on_debug_line_insts) {
#define DELEGATE(i) i->ForEachInst(f, run_on_debug_line_insts)
  for (auto& i : capabilities_) DELEGATE(i);
  for (auto& i : extensions_) DELEGATE(i);
  for (auto& i : ext_inst_imports_) DELEGATE(i);
  if (memory_model_) DELEGATE(memory_model_);
  for (auto& i : entry_points_) DELEGATE(i);
  for (auto& i : execution_modes_) DELEGATE(i);
  for (auto& i : debugs_) DELEGATE(i);
  for (auto& i : annotations_) DELEGATE(i);
  for (auto& i : types_values_) DELEGATE(i);
  for (auto& i : functions_) DELEGATE(i);
#undef DELEGATE
}

void Module::ForEachInst(const std::function<void(const Instruction*)>& f,
                         bool run_on_debug_line_insts) const {
#define DELEGATE(i)                                      \
  static_cast<const Instruction*>(i.get())->ForEachInst( \
      f, run_on_debug_line_insts)
  for (auto& i : capabilities_) DELEGATE(i);
  for (auto& i : extensions_) DELEGATE(i);
  for (auto& i : ext_inst_imports_) DELEGATE(i);
  if (memory_model_) DELEGATE(memory_model_);
  for (auto& i : entry_points_) DELEGATE(i);
  for (auto& i : execution_modes_) DELEGATE(i);
  for (auto& i : debugs_) DELEGATE(i);
  for (auto& i : annotations_) DELEGATE(i);
  for (auto& i : types_values_) DELEGATE(i);
  for (auto& i : functions_) {
    static_cast<const Function*>(i.get())->ForEachInst(f,
                                                       run_on_debug_line_insts);
  }
#undef DELEGATE
}

void Module::ToBinary(std::vector<uint32_t>* binary, bool skip_nop) const {
  binary->push_back(header_.magic_number);
  binary->push_back(header_.version);
  // TODO(antiagainst): should we change the generator number?
  binary->push_back(header_.generator);
  binary->push_back(header_.bound);
  binary->push_back(header_.reserved);

  auto write_inst = [binary, skip_nop](const Instruction* i) {
    if (!(skip_nop && i->IsNop())) i->ToBinaryWithoutAttachedDebugInsts(binary);
  };
  ForEachInst(write_inst, true);
}

uint32_t Module::ComputeIdBound() const {
  uint32_t highest = 0;

  ForEachInst(
      [&highest](const Instruction* inst) {
        for (const auto& operand : *inst) {
          if (spvIsIdType(operand.type)) {
            highest = std::max(highest, operand.words[0]);
          }
        }
      },
      true /* scan debug line insts as well */);

  return highest + 1;
}

bool Module::HasCapability(uint32_t cap) {
  for (auto& ci : capabilities_) {
    uint32_t tcap = ci->GetSingleWordOperand(0);
    if (tcap == cap) {
      return true;
    }
  }
  return false;
}

uint32_t Module::GetExtInstImportId(const char* extstr) {
  for (auto& ei : ext_inst_imports_)
    if (!strcmp(extstr, reinterpret_cast<const char*>(
        &ei->GetInOperand(0).words[0])))
      return ei->result_id();
  return 0;
}

}  // namespace ir
}  // namespace spvtools
