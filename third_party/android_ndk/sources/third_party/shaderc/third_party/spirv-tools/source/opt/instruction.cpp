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

#include "instruction.h"

#include <initializer_list>

#include "reflect.h"

namespace spvtools {
namespace ir {

Instruction::Instruction(const spv_parsed_instruction_t& inst,
                         std::vector<Instruction>&& dbg_line)
    : opcode_(static_cast<SpvOp>(inst.opcode)),
      type_id_(inst.type_id),
      result_id_(inst.result_id),
      dbg_line_insts_(std::move(dbg_line)) {
  assert((!IsDebugLineInst(opcode_) || dbg_line.empty()) &&
         "Op(No)Line attaching to Op(No)Line found");
  for (uint32_t i = 0; i < inst.num_operands; ++i) {
    const auto& current_payload = inst.operands[i];
    std::vector<uint32_t> words(
        inst.words + current_payload.offset,
        inst.words + current_payload.offset + current_payload.num_words);
    operands_.emplace_back(current_payload.type, std::move(words));
  }
}

Instruction::Instruction(SpvOp op, uint32_t ty_id, uint32_t res_id,
                         const std::vector<Operand>& in_operands)
    : opcode_(op), type_id_(ty_id), result_id_(res_id), operands_() {
  if (type_id_ != 0) {
    operands_.emplace_back(spv_operand_type_t::SPV_OPERAND_TYPE_TYPE_ID,
                           std::initializer_list<uint32_t>{type_id_});
  }
  if (result_id_ != 0) {
    operands_.emplace_back(spv_operand_type_t::SPV_OPERAND_TYPE_RESULT_ID,
                           std::initializer_list<uint32_t>{result_id_});
  }
  operands_.insert(operands_.end(), in_operands.begin(), in_operands.end());
}

Instruction::Instruction(Instruction&& that)
    : opcode_(that.opcode_),
      type_id_(that.type_id_),
      result_id_(that.result_id_),
      operands_(std::move(that.operands_)),
      dbg_line_insts_(std::move(that.dbg_line_insts_)) {}

Instruction& Instruction::operator=(Instruction&& that) {
  opcode_ = that.opcode_;
  type_id_ = that.type_id_;
  result_id_ = that.result_id_;
  operands_ = std::move(that.operands_);
  dbg_line_insts_ = std::move(that.dbg_line_insts_);
  return *this;
}

uint32_t Instruction::GetSingleWordOperand(uint32_t index) const {
  const auto& words = GetOperand(index).words;
  assert(words.size() == 1 && "expected the operand only taking one word");
  return words.front();
}

uint32_t Instruction::NumInOperandWords() const {
  uint32_t size = 0;
  for (uint32_t i = TypeResultIdCount(); i < operands_.size(); ++i)
    size += static_cast<uint32_t>(operands_[i].words.size());
  return size;
}

void Instruction::ToBinaryWithoutAttachedDebugInsts(
    std::vector<uint32_t>* binary) const {
  const uint32_t num_words = 1 + NumOperandWords();
  binary->push_back((num_words << 16) | static_cast<uint16_t>(opcode_));
  for (const auto& operand : operands_)
    binary->insert(binary->end(), operand.words.begin(), operand.words.end());
}

}  // namespace ir
}  // namespace spvtools
