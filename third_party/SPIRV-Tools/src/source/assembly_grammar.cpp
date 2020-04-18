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

#include "assembly_grammar.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "ext_inst.h"
#include "opcode.h"
#include "operand.h"
#include "table.h"

namespace {

/// @brief Parses a mask expression string for the given operand type.
///
/// A mask expression is a sequence of one or more terms separated by '|',
/// where each term a named enum value for the given type.  No whitespace
/// is permitted.
///
/// On success, the value is written to pValue.
///
/// @param[in] operandTable operand lookup table
/// @param[in] type of the operand
/// @param[in] textValue word of text to be parsed
/// @param[out] pValue where the resulting value is written
///
/// @return result code
spv_result_t spvTextParseMaskOperand(const spv_operand_table operandTable,
                                     const spv_operand_type_t type,
                                     const char* textValue, uint32_t* pValue) {
  if (textValue == nullptr) return SPV_ERROR_INVALID_TEXT;
  size_t text_length = strlen(textValue);
  if (text_length == 0) return SPV_ERROR_INVALID_TEXT;
  const char* text_end = textValue + text_length;

  // We only support mask expressions in ASCII, so the separator value is a
  // char.
  const char separator = '|';

  // Accumulate the result by interpreting one word at a time, scanning
  // from left to right.
  uint32_t value = 0;
  const char* begin = textValue;  // The left end of the current word.
  const char* end = nullptr;  // One character past the end of the current word.
  do {
    end = std::find(begin, text_end, separator);

    spv_operand_desc entry = nullptr;
    if (spvOperandTableNameLookup(operandTable, type, begin, end - begin,
                                  &entry)) {
      return SPV_ERROR_INVALID_TEXT;
    }
    value |= entry->value;

    // Advance to the next word by skipping over the separator.
    begin = end + 1;
  } while (end != text_end);

  *pValue = value;
  return SPV_SUCCESS;
}

// Associates an opcode with its name.
struct SpecConstantOpcodeEntry {
  SpvOp opcode;
  const char* name;
};

// All the opcodes allowed as the operation for OpSpecConstantOp.
// The name does not have the usual "Op" prefix. For example opcode SpvOpIAdd
// is associated with the name "IAdd".
//
// clang-format off
#define CASE(NAME) { SpvOp##NAME, #NAME }
const SpecConstantOpcodeEntry kOpSpecConstantOpcodes[] = {
    // Conversion
    CASE(SConvert),
    CASE(FConvert),
    CASE(ConvertFToS),
    CASE(ConvertSToF),
    CASE(ConvertFToU),
    CASE(ConvertUToF),
    CASE(UConvert),
    CASE(ConvertPtrToU),
    CASE(ConvertUToPtr),
    CASE(GenericCastToPtr),
    CASE(PtrCastToGeneric),
    CASE(Bitcast),
    CASE(QuantizeToF16),
    // Arithmetic
    CASE(SNegate),
    CASE(Not),
    CASE(IAdd),
    CASE(ISub),
    CASE(IMul),
    CASE(UDiv),
    CASE(SDiv),
    CASE(UMod),
    CASE(SRem),
    CASE(SMod),
    CASE(ShiftRightLogical),
    CASE(ShiftRightArithmetic),
    CASE(ShiftLeftLogical),
    CASE(BitwiseOr),
    CASE(BitwiseAnd),
    CASE(BitwiseXor),
    CASE(FNegate),
    CASE(FAdd),
    CASE(FSub),
    CASE(FMul),
    CASE(FDiv),
    CASE(FRem),
    CASE(FMod),
    // Composite
    CASE(VectorShuffle),
    CASE(CompositeExtract),
    CASE(CompositeInsert),
    // Logical
    CASE(LogicalOr),
    CASE(LogicalAnd),
    CASE(LogicalNot),
    CASE(LogicalEqual),
    CASE(LogicalNotEqual),
    CASE(Select),
    // Comparison
    CASE(IEqual),
    CASE(ULessThan),
    CASE(SLessThan),
    CASE(UGreaterThan),
    CASE(SGreaterThan),
    CASE(ULessThanEqual),
    CASE(SLessThanEqual),
    CASE(UGreaterThanEqual),
    CASE(SGreaterThanEqual),
    // Memory
    CASE(AccessChain),
    CASE(InBoundsAccessChain),
    CASE(PtrAccessChain),
    CASE(InBoundsPtrAccessChain),
};

// The 58 is determined by counting the opcodes listed in the spec.
static_assert(58 == sizeof(kOpSpecConstantOpcodes)/sizeof(kOpSpecConstantOpcodes[0]),
              "OpSpecConstantOp opcode table is incomplete");
#undef CASE
// clang-format on

const size_t kNumOpSpecConstantOpcodes =
    sizeof(kOpSpecConstantOpcodes) / sizeof(kOpSpecConstantOpcodes[0]);

}  // anonymous namespace

namespace libspirv {

bool AssemblyGrammar::isValid() const {
  return operandTable_ && opcodeTable_ && extInstTable_;
}

spv_result_t AssemblyGrammar::lookupOpcode(const char* name,
                                           spv_opcode_desc* desc) const {
  return spvOpcodeTableNameLookup(opcodeTable_, name, desc);
}

spv_result_t AssemblyGrammar::lookupOpcode(SpvOp opcode,
                                           spv_opcode_desc* desc) const {
  return spvOpcodeTableValueLookup(opcodeTable_, opcode, desc);
}

spv_result_t AssemblyGrammar::lookupOperand(spv_operand_type_t type,
                                            const char* name, size_t name_len,
                                            spv_operand_desc* desc) const {
  return spvOperandTableNameLookup(operandTable_, type, name, name_len, desc);
}

spv_result_t AssemblyGrammar::lookupOperand(spv_operand_type_t type,
                                            uint32_t operand,
                                            spv_operand_desc* desc) const {
  return spvOperandTableValueLookup(operandTable_, type, operand, desc);
}

spv_result_t AssemblyGrammar::lookupSpecConstantOpcode(const char* name,
                                                       SpvOp* opcode) const {
  const auto* last = kOpSpecConstantOpcodes + kNumOpSpecConstantOpcodes;
  const auto* found =
      std::find_if(kOpSpecConstantOpcodes, last,
                   [name](const SpecConstantOpcodeEntry& entry) {
                     return 0 == strcmp(name, entry.name);
                   });
  if (found == last) return SPV_ERROR_INVALID_LOOKUP;
  *opcode = found->opcode;
  return SPV_SUCCESS;
}

spv_result_t AssemblyGrammar::lookupSpecConstantOpcode(SpvOp opcode) const {
  const auto* last = kOpSpecConstantOpcodes + kNumOpSpecConstantOpcodes;
  const auto* found =
      std::find_if(kOpSpecConstantOpcodes, last,
                   [opcode](const SpecConstantOpcodeEntry& entry) {
                     return opcode == entry.opcode;
                   });
  if (found == last) return SPV_ERROR_INVALID_LOOKUP;
  return SPV_SUCCESS;
}

spv_result_t AssemblyGrammar::parseMaskOperand(const spv_operand_type_t type,
                                               const char* textValue,
                                               uint32_t* pValue) const {
  return spvTextParseMaskOperand(operandTable_, type, textValue, pValue);
}
spv_result_t AssemblyGrammar::lookupExtInst(spv_ext_inst_type_t type,
                                            const char* textValue,
                                            spv_ext_inst_desc* extInst) const {
  return spvExtInstTableNameLookup(extInstTable_, type, textValue, extInst);
}

spv_result_t AssemblyGrammar::lookupExtInst(spv_ext_inst_type_t type,
                                            uint32_t firstWord,
                                            spv_ext_inst_desc* extInst) const {
  return spvExtInstTableValueLookup(extInstTable_, type, firstWord, extInst);
}

void AssemblyGrammar::prependOperandTypesForMask(
    const spv_operand_type_t type, const uint32_t mask,
    spv_operand_pattern_t* pattern) const {
  spvPrependOperandTypesForMask(operandTable_, type, mask, pattern);
}
}  // namespace libspirv
