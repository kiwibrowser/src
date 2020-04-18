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

#ifndef LIBSPIRV_ASSEMBLY_GRAMMAR_H_
#define LIBSPIRV_ASSEMBLY_GRAMMAR_H_

#include "operand.h"
#include "spirv-tools/libspirv.h"
#include "spirv/spirv.h"
#include "table.h"

namespace libspirv {

// Encapsulates the grammar to use for SPIR-V assembly.
// Contains methods to query for valid instructions and operands.
class AssemblyGrammar {
 public:
  explicit AssemblyGrammar(const spv_const_context context)
      : target_env_(context->target_env),
        operandTable_(context->operand_table),
        opcodeTable_(context->opcode_table),
        extInstTable_(context->ext_inst_table) {}

  // Returns true if the internal tables have been initialized with valid data.
  bool isValid() const;

  // Returns the SPIR-V target environment.
  spv_target_env target_env() const { return target_env_; }

  // Fills in the desc parameter with the information about the opcode
  // of the given name. Returns SPV_SUCCESS if the opcode was found, and
  // SPV_ERROR_INVALID_LOOKUP if the opcode does not exist.
  spv_result_t lookupOpcode(const char* name, spv_opcode_desc* desc) const;

  // Fills in the desc parameter with the information about the opcode
  // of the valid. Returns SPV_SUCCESS if the opcode was found, and
  // SPV_ERROR_INVALID_LOOKUP if the opcode does not exist.
  spv_result_t lookupOpcode(SpvOp opcode, spv_opcode_desc* desc) const;

  // Fills in the desc parameter with the information about the given
  // operand. Returns SPV_SUCCESS if the operand was found, and
  // SPV_ERROR_INVALID_LOOKUP otherwise.
  spv_result_t lookupOperand(spv_operand_type_t type, const char* name,
                             size_t name_len, spv_operand_desc* desc) const;

  // Fills in the desc parameter with the information about the given
  // operand. Returns SPV_SUCCESS if the operand was found, and
  // SPV_ERROR_INVALID_LOOKUP otherwise.
  spv_result_t lookupOperand(spv_operand_type_t type, uint32_t operand,
                             spv_operand_desc* desc) const;

  // Finds the opcode for the given OpSpecConstantOp opcode name. The name
  // should not have the "Op" prefix.  For example, "IAdd" corresponds to
  // the integer add opcode for OpSpecConstantOp.  On success, returns
  // SPV_SUCCESS and sends the discovered operation code through the opcode
  // parameter.  On failure, returns SPV_ERROR_INVALID_LOOKUP.
  spv_result_t lookupSpecConstantOpcode(const char* name, SpvOp* opcode) const;

  // Returns SPV_SUCCESS if the given opcode is valid as the opcode operand
  // to OpSpecConstantOp.
  spv_result_t lookupSpecConstantOpcode(SpvOp opcode) const;

  // Parses a mask expression string for the given operand type.
  //
  // A mask expression is a sequence of one or more terms separated by '|',
  // where each term is a named enum value for a given type. No whitespace
  // is permitted.
  //
  // On success, the value is written to pValue, and SPV_SUCCESS is returned.
  // The operand type is defined by the type parameter, and the text to be
  // parsed is defined by the textValue parameter.
  spv_result_t parseMaskOperand(const spv_operand_type_t type,
                                const char* textValue, uint32_t* pValue) const;

  // Writes the extended operand with the given type and text to the *extInst
  // parameter.
  // Returns SPV_SUCCESS if the value could be found.
  spv_result_t lookupExtInst(spv_ext_inst_type_t type, const char* textValue,
                             spv_ext_inst_desc* extInst) const;

  // Writes the extended operand with the given type and first encoded word
  // to the *extInst parameter.
  // Returns SPV_SUCCESS if the value could be found.
  spv_result_t lookupExtInst(spv_ext_inst_type_t type, uint32_t firstWord,
                             spv_ext_inst_desc* extInst) const;

  // Inserts the operands expected after the given typed mask onto the front
  // of the given pattern.
  //
  // Each set bit in the mask represents zero or more operand types that should
  // be prepended onto the pattern. Operands for a less significant bit always
  // appear before operands for a more significant bit.
  //
  // If a set bit is unknown, then we assume it has no operands.
  void prependOperandTypesForMask(const spv_operand_type_t type,
                                  const uint32_t mask,
                                  spv_operand_pattern_t* pattern) const;

 private:
  const spv_target_env target_env_;
  const spv_operand_table operandTable_;
  const spv_opcode_table opcodeTable_;
  const spv_ext_inst_table extInstTable_;
};
}

#endif  // LIBSPIRV_ASSEMBLY_GRAMMAR_H_
