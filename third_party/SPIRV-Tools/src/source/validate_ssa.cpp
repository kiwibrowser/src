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

#include <functional>
#include "opcode.h"
#include "validate_passes.h"

using std::function;
using libspirv::ValidationState_t;

namespace {
// This function takes the opcode of an instruction and returns
// a function object that will return true if the index
// of the operand can be forwarad declared. This function will
// used in the SSA validation stage of the pipeline
function<bool(unsigned)> getCanBeForwardDeclaredFunction(SpvOp opcode) {
  function<bool(unsigned index)> out;
  switch (opcode) {
    case SpvOpExecutionMode:
    case SpvOpEntryPoint:
    case SpvOpName:
    case SpvOpMemberName:
    case SpvOpSelectionMerge:
    case SpvOpDecorate:
    case SpvOpMemberDecorate:
    case SpvOpBranch:
    case SpvOpLoopMerge:
      out = [](unsigned) { return true; };
      break;
    case SpvOpGroupDecorate:
    case SpvOpGroupMemberDecorate:
    case SpvOpBranchConditional:
    case SpvOpSwitch:
      out = [](unsigned index) { return index != 0; };
      break;

    case SpvOpFunctionCall:
      out = [](unsigned index) { return index == 2; };
      break;

    case SpvOpPhi:
      out = [](unsigned index) { return index > 1; };
      break;

    case SpvOpEnqueueKernel:
      out = [](unsigned index) { return index == 8; };
      break;

    case SpvOpGetKernelNDrangeSubGroupCount:
    case SpvOpGetKernelNDrangeMaxSubGroupSize:
      out = [](unsigned index) { return index == 3; };
      break;

    case SpvOpGetKernelWorkGroupSize:
    case SpvOpGetKernelPreferredWorkGroupSizeMultiple:
      out = [](unsigned index) { return index == 2; };
      break;

    default:
      out = [](unsigned) { return false; };
      break;
  }
  return out;
}
}

namespace libspirv {

// Performs SSA validation on the IDs of an instruction. The
// can_have_forward_declared_ids  functor should return true if the
// instruction operand's ID can be forward referenced.
//
// TODO(umar): Use dominators to correctly validate SSA. For example, the result
// id from a 'then' block cannot dominate its usage in the 'else' block. This
// is not yet performed by this function.
spv_result_t SsaPass(ValidationState_t& _,
                     const spv_parsed_instruction_t* inst) {
  auto can_have_forward_declared_ids =
      getCanBeForwardDeclaredFunction(static_cast<SpvOp>(inst->opcode));

  for (unsigned i = 0; i < inst->num_operands; i++) {
    const spv_parsed_operand_t& operand = inst->operands[i];
    const spv_operand_type_t& type = operand.type;
    const uint32_t* operand_ptr = inst->words + operand.offset;

    auto ret = SPV_ERROR_INTERNAL;
    switch (type) {
      case SPV_OPERAND_TYPE_RESULT_ID:
        _.removeIfForwardDeclared(*operand_ptr);
        ret = SPV_SUCCESS;
        break;
      case SPV_OPERAND_TYPE_ID:
      case SPV_OPERAND_TYPE_TYPE_ID:
      case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID:
      case SPV_OPERAND_TYPE_SCOPE_ID:
        if (_.isDefinedId(*operand_ptr)) {
          ret = SPV_SUCCESS;
        } else if (can_have_forward_declared_ids(i)) {
          ret = _.forwardDeclareId(*operand_ptr);
        } else {
          ret = _.diag(SPV_ERROR_INVALID_ID) << "ID "
                                             << _.getIdName(*operand_ptr)
                                             << " has not been defined";
        }
        break;
      default:
        ret = SPV_SUCCESS;
        break;
    }
    if (SPV_SUCCESS != ret) {
      return ret;
    }
  }
  return SPV_SUCCESS;
}
}
