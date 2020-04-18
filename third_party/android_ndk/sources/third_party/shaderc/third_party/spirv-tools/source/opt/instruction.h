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

#ifndef LIBSPIRV_OPT_INSTRUCTION_H_
#define LIBSPIRV_OPT_INSTRUCTION_H_

#include <cassert>
#include <functional>
#include <utility>
#include <vector>

#include "operand.h"

#include "spirv-tools/libspirv.h"
#include "spirv/1.2/spirv.h"

namespace spvtools {
namespace ir {

class Function;
class Module;

// About operand:
//
// In the SPIR-V specification, the term "operand" is used to mean any single
// SPIR-V word following the leading wordcount-opcode word. Here, the term
// "operand" is used to mean a *logical* operand. A logical operand may consist
// of mulitple SPIR-V words, which together make up the same component. For
// example, a logical operand of a 64-bit integer needs two words to express.
//
// Further, we categorize logical operands into *in* and *out* operands.
// In operands are operands actually serve as input to operations, while out
// operands are operands that represent ids generated from operations (result
// type id or result id). For example, for "OpIAdd %rtype %rid %inop1 %inop2",
// "%inop1" and "%inop2" are in operands, while "%rtype" and "%rid" are out
// operands.

// A *logical* operand to a SPIR-V instruction. It can be the type id, result
// id, or other additional operands carried in an instruction.
struct Operand {
  Operand(spv_operand_type_t t, std::vector<uint32_t>&& w)
      : type(t), words(std::move(w)) {}

  Operand(spv_operand_type_t t, const std::vector<uint32_t>& w)
      : type(t), words(w) {}

  spv_operand_type_t type;      // Type of this logical operand.
  std::vector<uint32_t> words;  // Binary segments of this logical operand.

  // TODO(antiagainst): create fields for literal number kind, width, etc.
};

// A SPIR-V instruction. It contains the opcode and any additional logical
// operand, including the result id (if any) and result type id (if any). It
// may also contain line-related debug instruction (OpLine, OpNoLine) directly
// appearing before this instruction. Note that the result id of an instruction
// should never change after the instruction being built. If the result id
// needs to change, the user should create a new instruction instead.
class Instruction {
 public:
  using iterator = std::vector<Operand>::iterator;
  using const_iterator = std::vector<Operand>::const_iterator;

  // Creates a default OpNop instruction.
  Instruction() : opcode_(SpvOpNop), type_id_(0), result_id_(0) {}
  // Creates an instruction with the given opcode |op| and no additional logical
  // operands.
  Instruction(SpvOp op) : opcode_(op), type_id_(0), result_id_(0) {}
  // Creates an instruction using the given spv_parsed_instruction_t |inst|. All
  // the data inside |inst| will be copied and owned in this instance. And keep
  // record of line-related debug instructions |dbg_line| ahead of this
  // instruction, if any.
  Instruction(const spv_parsed_instruction_t& inst,
              std::vector<Instruction>&& dbg_line = {});

  // Creates an instruction with the given opcode |op|, type id: |ty_id|,
  // result id: |res_id| and input operands: |in_operands|.
  Instruction(SpvOp op, uint32_t ty_id, uint32_t res_id,
              const std::vector<Operand>& in_operands);

  Instruction(const Instruction&) = default;
  Instruction& operator=(const Instruction&) = default;

  Instruction(Instruction&&);
  Instruction& operator=(Instruction&&);

  SpvOp opcode() const { return opcode_; }
  // Sets the opcode of this instruction to a specific opcode. Note this may
  // invalidate the instruction.
  // TODO(qining): Remove this function when instruction building and insertion
  // is well implemented.
  void SetOpcode(SpvOp op) { opcode_ = op; }
  uint32_t type_id() const { return type_id_; }
  uint32_t result_id() const { return result_id_; }
  // Returns the vector of line-related debug instructions attached to this
  // instruction and the caller can directly modify them.
  std::vector<Instruction>& dbg_line_insts() { return dbg_line_insts_; }
  const std::vector<Instruction>& dbg_line_insts() const {
    return dbg_line_insts_;
  }

  // Begin and end iterators for operands.
  iterator begin() { return operands_.begin(); }
  iterator end() { return operands_.end(); }
  const_iterator begin() const { return operands_.cbegin(); }
  const_iterator end() const { return operands_.cend(); }
  // Const begin and end iterators for operands.
  const_iterator cbegin() const { return operands_.cbegin(); }
  const_iterator cend() const { return operands_.cend(); }

  // Gets the number of logical operands.
  uint32_t NumOperands() const {
    return static_cast<uint32_t>(operands_.size());
  }
  // Gets the number of SPIR-V words occupied by all logical operands.
  uint32_t NumOperandWords() const {
    return NumInOperandWords() + TypeResultIdCount();
  }
  // Gets the |index|-th logical operand.
  inline const Operand& GetOperand(uint32_t index) const;
  // Gets the |index|-th logical operand as a single SPIR-V word. This method is
  // not expected to be used with logical operands consisting of multiple SPIR-V
  // words.
  uint32_t GetSingleWordOperand(uint32_t index) const;
  // Sets the |index|-th in-operand's data to the given |data|.
  inline void SetInOperand(uint32_t index, std::vector<uint32_t>&& data);
  // Sets the result type id.
  inline void SetResultType(uint32_t ty_id);
  // Sets the result id
  inline void SetResultId(uint32_t res_id);

  // The following methods are similar to the above, but are for in operands.
  uint32_t NumInOperands() const {
    return static_cast<uint32_t>(operands_.size() - TypeResultIdCount());
  }
  uint32_t NumInOperandWords() const;
  const Operand& GetInOperand(uint32_t index) const {
    return GetOperand(index + TypeResultIdCount());
  }
  uint32_t GetSingleWordInOperand(uint32_t index) const {
    return GetSingleWordOperand(index + TypeResultIdCount());
  }

  // Returns true if this instruction is OpNop.
  inline bool IsNop() const;
  // Turns this instruction to OpNop. This does not clear out all preceding
  // line-related debug instructions.
  inline void ToNop();

  // Runs the given function |f| on this instruction and optionally on the
  // preceding debug line instructions.  The function will always be run
  // if this is itself a debug line instruction.
  inline void ForEachInst(const std::function<void(Instruction*)>& f,
                          bool run_on_debug_line_insts = false);
  inline void ForEachInst(const std::function<void(const Instruction*)>& f,
                          bool run_on_debug_line_insts = false) const;

  // Runs the given function |f| on all "in" operand ids
  inline void ForEachInId(const std::function<void(uint32_t*)>& f);
  inline void ForEachInId(const std::function<void(const uint32_t*)>& f) const;

  // Returns true if any operands can be labels
  inline bool HasLabels() const;

  // Pushes the binary segments for this instruction into the back of *|binary|.
  void ToBinaryWithoutAttachedDebugInsts(std::vector<uint32_t>* binary) const;

 private:
  // Returns the toal count of result type id and result id.
  uint32_t TypeResultIdCount() const {
    return (type_id_ != 0) + (result_id_ != 0);
  }

  SpvOp opcode_;        // Opcode
  uint32_t type_id_;    // Result type id. A value of 0 means no result type id.
  uint32_t result_id_;  // Result id. A value of 0 means no result id.
  // All logical operands, including result type id and result id.
  std::vector<Operand> operands_;
  // Opline and OpNoLine instructions preceding this instruction. Note that for
  // Instructions representing OpLine or OpNonLine itself, this field should be
  // empty.
  std::vector<Instruction> dbg_line_insts_;
};

inline const Operand& Instruction::GetOperand(uint32_t index) const {
  assert(index < operands_.size() && "operand index out of bound");
  return operands_[index];
};

inline void Instruction::SetInOperand(uint32_t index,
                                      std::vector<uint32_t>&& data) {
  assert(index + TypeResultIdCount() < operands_.size() &&
         "operand index out of bound");
  operands_[index + TypeResultIdCount()].words = std::move(data);
}

inline void Instruction::SetResultId(uint32_t res_id) {
  result_id_ = res_id;
  auto ridx = (type_id_ != 0) ? 1 : 0;
  assert(operands_[ridx].type == SPV_OPERAND_TYPE_RESULT_ID);
  operands_[ridx].words = {res_id};
}

inline void Instruction::SetResultType(uint32_t ty_id) {
  if (type_id_ != 0) {
    type_id_ = ty_id;
    assert(operands_.front().type == SPV_OPERAND_TYPE_TYPE_ID);
    operands_.front().words = {ty_id};
  }
}

inline bool Instruction::IsNop() const {
  return opcode_ == SpvOpNop && type_id_ == 0 && result_id_ == 0 &&
         operands_.empty();
}

inline void Instruction::ToNop() {
  opcode_ = SpvOpNop;
  type_id_ = result_id_ = 0;
  operands_.clear();
}

inline void Instruction::ForEachInst(const std::function<void(Instruction*)>& f,
                                     bool run_on_debug_line_insts) {
  if (run_on_debug_line_insts)
    for (auto& dbg_line : dbg_line_insts_) f(&dbg_line);
  f(this);
}

inline void Instruction::ForEachInst(
    const std::function<void(const Instruction*)>& f,
    bool run_on_debug_line_insts) const {
  if (run_on_debug_line_insts)
    for (auto& dbg_line : dbg_line_insts_) f(&dbg_line);
  f(this);
}

inline void Instruction::ForEachInId(const std::function<void(uint32_t*)>& f) {
  for (auto& opnd : operands_) {
    switch (opnd.type) {
      case SPV_OPERAND_TYPE_RESULT_ID:
      case SPV_OPERAND_TYPE_TYPE_ID:
        break;
      default:
        if (spvIsIdType(opnd.type)) f(&opnd.words[0]);
        break;
    }
  }
}

inline void Instruction::ForEachInId(
    const std::function<void(const uint32_t*)>& f) const {
  for (const auto& opnd : operands_) {
    switch (opnd.type) {
      case SPV_OPERAND_TYPE_RESULT_ID:
      case SPV_OPERAND_TYPE_TYPE_ID:
        break;
      default:
        if (spvIsIdType(opnd.type)) f(&opnd.words[0]);
        break;
    }
  }
}

inline bool Instruction::HasLabels() const {
  switch (opcode_) {
    case SpvOpSelectionMerge:
    case SpvOpBranch:
    case SpvOpLoopMerge:
    case SpvOpBranchConditional:
    case SpvOpSwitch:
    case SpvOpPhi:
      return true;
      break;
    default:
      break;
  }
  return false;
}

}  // namespace ir
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_INSTRUCTION_H_
