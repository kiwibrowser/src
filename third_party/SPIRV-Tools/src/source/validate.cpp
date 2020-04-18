// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include "validate.h"
#include "validate_passes.h"

#include "binary.h"
#include "diagnostic.h"
#include "instruction.h"
#include "opcode.h"
#include "operand.h"
#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"
#include "spirv_endian.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using std::function;
using std::ostream_iterator;
using std::placeholders::_1;
using std::string;
using std::stringstream;
using std::transform;
using std::vector;

using libspirv::CfgPass;
using libspirv::InstructionPass;
using libspirv::ModuleLayoutPass;
using libspirv::SsaPass;
using libspirv::ValidationState_t;

spv_result_t spvValidateIDs(
    const spv_instruction_t* pInsts, const uint64_t count,
    const spv_opcode_table opcodeTable, const spv_operand_table operandTable,
    const spv_ext_inst_table extInstTable, const ValidationState_t& state,
    spv_position position, spv_diagnostic* pDiagnostic) {
  auto undefd = state.usedefs().FindUsesWithoutDefs();
  for (auto id : undefd) {
    DIAGNOSTIC << "Undefined ID: " << id;
  }
  position->index = SPV_INDEX_INSTRUCTION;
  spvCheckReturn(spvValidateInstructionIDs(pInsts, count, opcodeTable,
                                           operandTable, extInstTable, state,
                                           position, pDiagnostic));
  return undefd.empty() ? SPV_SUCCESS : SPV_ERROR_INVALID_ID;
}

namespace {

// TODO(umar): Validate header
// TODO(umar): The Id bound should be validated also. But you can only do that
// after you've seen all the instructions in the module.
// TODO(umar): The binary parser validates the magic word, and the length of the
// header, but nothing else.
spv_result_t setHeader(void* user_data, spv_endianness_t endian, uint32_t magic,
                       uint32_t version, uint32_t generator, uint32_t id_bound,
                       uint32_t reserved) {
  (void)user_data;
  (void)endian;
  (void)magic;
  (void)version;
  (void)generator;
  (void)id_bound;
  (void)reserved;
  return SPV_SUCCESS;
}

// Improves diagnostic messages by collecting names of IDs
// NOTE: This function returns void and is not involved in validation
void DebugInstructionPass(ValidationState_t& _,
                          const spv_parsed_instruction_t* inst) {
  switch (inst->opcode) {
    case SpvOpName: {
      const uint32_t target = *(inst->words + inst->operands[0].offset);
      const char* str =
          reinterpret_cast<const char*>(inst->words + inst->operands[1].offset);
      _.assignNameToId(target, str);
    } break;
    case SpvOpMemberName: {
      const uint32_t target = *(inst->words + inst->operands[0].offset);
      const char* str =
          reinterpret_cast<const char*>(inst->words + inst->operands[2].offset);
      _.assignNameToId(target, str);
    } break;
    case SpvOpSourceContinued:
    case SpvOpSource:
    case SpvOpSourceExtension:
    case SpvOpString:
    case SpvOpLine:
    case SpvOpNoLine:

    default:
      break;
  }
}

// Collects use-def info about an instruction's IDs.
void ProcessIds(ValidationState_t& _, const spv_parsed_instruction_t& inst) {
  if (inst.result_id) {
    _.usedefs().AddDef(
        {inst.result_id, inst.type_id, static_cast<SpvOp>(inst.opcode),
         std::vector<uint32_t>(inst.words, inst.words + inst.num_words)});
  }
  for (auto op = inst.operands; op != inst.operands + inst.num_operands; ++op) {
    if (spvIsIdType(op->type)) _.usedefs().AddUse(inst.words[op->offset]);
  }
}

spv_result_t ProcessInstruction(void* user_data,
                                const spv_parsed_instruction_t* inst) {
  ValidationState_t& _ = *(reinterpret_cast<ValidationState_t*>(user_data));
  _.incrementInstructionCount();
  if (static_cast<SpvOp>(inst->opcode) == SpvOpEntryPoint)
    _.entry_points().push_back(inst->words[2]);

  DebugInstructionPass(_, inst);
  // TODO(umar): Perform data rules pass
  ProcessIds(_, *inst);
  spvCheckReturn(ModuleLayoutPass(_, inst));
  spvCheckReturn(CfgPass(_, inst));
  spvCheckReturn(SsaPass(_, inst));
  spvCheckReturn(InstructionPass(_, inst));

  return SPV_SUCCESS;
}

}  // anonymous namespace

spv_result_t spvValidate(const spv_const_context context,
                         const spv_const_binary binary,
                         spv_diagnostic* pDiagnostic) {
  if (!pDiagnostic) return SPV_ERROR_INVALID_DIAGNOSTIC;

  spv_endianness_t endian;
  spv_position_t position = {};
  if (spvBinaryEndianness(binary, &endian)) {
    DIAGNOSTIC << "Invalid SPIR-V magic number.";
    return SPV_ERROR_INVALID_BINARY;
  }

  spv_header_t header;
  if (spvBinaryHeaderGet(binary, endian, &header)) {
    DIAGNOSTIC << "Invalid SPIR-V header.";
    return SPV_ERROR_INVALID_BINARY;
  }

  // NOTE: Parse the module and perform inline validation checks. These
  // checks do not require the the knowledge of the whole module.
  ValidationState_t vstate(pDiagnostic, context);
  spvCheckReturn(spvBinaryParse(context, &vstate, binary->code,
                                binary->wordCount, setHeader,
                                ProcessInstruction, pDiagnostic));

  // TODO(umar): Add validation checks which require the parsing of the entire
  // module. Use the information from the ProcessInstruction pass to make the
  // checks.

  if (vstate.unresolvedForwardIdCount() > 0) {
    stringstream ss;
    vector<uint32_t> ids = vstate.unresolvedForwardIds();

    transform(begin(ids), end(ids), ostream_iterator<string>(ss, " "),
              bind(&ValidationState_t::getIdName, vstate, _1));

    auto id_str = ss.str();
    return vstate.diag(SPV_ERROR_INVALID_ID)
           << "The following forward referenced IDs have not be defined:\n"
           << id_str.substr(0, id_str.size() - 1);
  }

  // NOTE: Copy each instruction for easier processing
  std::vector<spv_instruction_t> instructions;
  uint64_t index = SPV_INDEX_INSTRUCTION;
  while (index < binary->wordCount) {
    uint16_t wordCount;
    uint16_t opcode;
    spvOpcodeSplit(spvFixWord(binary->code[index], endian), &wordCount,
                   &opcode);
    spv_instruction_t inst;
    spvInstructionCopy(&binary->code[index], static_cast<SpvOp>(opcode),
                       wordCount, endian, &inst);
    instructions.push_back(inst);
    index += wordCount;
  }

  position.index = SPV_INDEX_INSTRUCTION;
  spvCheckReturn(spvValidateIDs(instructions.data(), instructions.size(),
                                context->opcode_table, context->operand_table,
                                context->ext_inst_table, vstate, &position,
                                pDiagnostic));

  return SPV_SUCCESS;
}
