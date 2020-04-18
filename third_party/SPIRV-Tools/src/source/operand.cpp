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

#include "operand.h"

#include <assert.h>
#include <string.h>

// Evaluates to the number of elements of array A.
// If we could use constexpr, then we could make this a template function.
// If the source arrays were std::array, then we could have used
// std::array::size.
#define ARRAY_SIZE(A) (static_cast<uint32_t>(sizeof(A) / sizeof(A[0])))

// Pull in operand info tables automatically generated from JSON grammar.
namespace v1_0 {
#include "operand.kinds-1-0.inc"
}  // namespace v1_0
namespace v1_1 {
#include "operand.kinds-1-1.inc"
}  // namespace v1_1

spv_result_t spvOperandTableGet(spv_operand_table* pOperandTable,
                                spv_target_env env) {
  if (!pOperandTable) return SPV_ERROR_INVALID_POINTER;

  static const spv_operand_table_t table_1_0 = {
      ARRAY_SIZE(v1_0::pygen_variable_OperandInfoTable),
      v1_0::pygen_variable_OperandInfoTable};
  static const spv_operand_table_t table_1_1 = {
      ARRAY_SIZE(v1_1::pygen_variable_OperandInfoTable),
      v1_1::pygen_variable_OperandInfoTable};

  switch (env) {
    case SPV_ENV_UNIVERSAL_1_0:
    case SPV_ENV_VULKAN_1_0:
      *pOperandTable = &table_1_0;
      return SPV_SUCCESS;
    case SPV_ENV_UNIVERSAL_1_1:
      *pOperandTable = &table_1_1;
      return SPV_SUCCESS;
  }
  assert(0 && "Unknown spv_target_env in spvOperandTableGet()");
  return SPV_ERROR_INVALID_TABLE;
}

#undef ARRAY_SIZE

spv_result_t spvOperandTableNameLookup(const spv_operand_table table,
                                       const spv_operand_type_t type,
                                       const char* name,
                                       const size_t nameLength,
                                       spv_operand_desc* pEntry) {
  if (!table) return SPV_ERROR_INVALID_TABLE;
  if (!name || !pEntry) return SPV_ERROR_INVALID_POINTER;

  for (uint64_t typeIndex = 0; typeIndex < table->count; ++typeIndex) {
    if (type == table->types[typeIndex].type) {
      for (uint64_t operandIndex = 0;
           operandIndex < table->types[typeIndex].count; ++operandIndex) {
        if (nameLength ==
                strlen(table->types[typeIndex].entries[operandIndex].name) &&
            !strncmp(table->types[typeIndex].entries[operandIndex].name, name,
                     nameLength)) {
          *pEntry = &table->types[typeIndex].entries[operandIndex];
          return SPV_SUCCESS;
        }
      }
    }
  }

  return SPV_ERROR_INVALID_LOOKUP;
}

spv_result_t spvOperandTableValueLookup(const spv_operand_table table,
                                        const spv_operand_type_t type,
                                        const uint32_t value,
                                        spv_operand_desc* pEntry) {
  if (!table) return SPV_ERROR_INVALID_TABLE;
  if (!pEntry) return SPV_ERROR_INVALID_POINTER;

  for (uint64_t typeIndex = 0; typeIndex < table->count; ++typeIndex) {
    if (type == table->types[typeIndex].type) {
      for (uint64_t operandIndex = 0;
           operandIndex < table->types[typeIndex].count; ++operandIndex) {
        if (value == table->types[typeIndex].entries[operandIndex].value) {
          *pEntry = &table->types[typeIndex].entries[operandIndex];
          return SPV_SUCCESS;
        }
      }
    }
  }

  return SPV_ERROR_INVALID_LOOKUP;
}

const char* spvOperandTypeStr(spv_operand_type_t type) {
  switch (type) {
    case SPV_OPERAND_TYPE_ID:
    case SPV_OPERAND_TYPE_OPTIONAL_ID:
      return "ID";
    case SPV_OPERAND_TYPE_TYPE_ID:
      return "type ID";
    case SPV_OPERAND_TYPE_RESULT_ID:
      return "result ID";
    case SPV_OPERAND_TYPE_LITERAL_INTEGER:
    case SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER:
    case SPV_OPERAND_TYPE_OPTIONAL_LITERAL_NUMBER:
      return "literal number";
    case SPV_OPERAND_TYPE_OPTIONAL_TYPED_LITERAL_INTEGER:
      return "possibly multi-word literal integer";
    case SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER:
      return "possibly multi-word literal number";
    case SPV_OPERAND_TYPE_EXTENSION_INSTRUCTION_NUMBER:
      return "extension instruction number";
    case SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER:
      return "OpSpecConstantOp opcode";
    case SPV_OPERAND_TYPE_LITERAL_STRING:
    case SPV_OPERAND_TYPE_OPTIONAL_LITERAL_STRING:
      return "literal string";
    case SPV_OPERAND_TYPE_SOURCE_LANGUAGE:
      return "source language";
    case SPV_OPERAND_TYPE_EXECUTION_MODEL:
      return "execution model";
    case SPV_OPERAND_TYPE_ADDRESSING_MODEL:
      return "addressing model";
    case SPV_OPERAND_TYPE_MEMORY_MODEL:
      return "memory model";
    case SPV_OPERAND_TYPE_EXECUTION_MODE:
      return "execution mode";
    case SPV_OPERAND_TYPE_STORAGE_CLASS:
      return "storage class";
    case SPV_OPERAND_TYPE_DIMENSIONALITY:
      return "dimensionality";
    case SPV_OPERAND_TYPE_SAMPLER_ADDRESSING_MODE:
      return "sampler addressing mode";
    case SPV_OPERAND_TYPE_SAMPLER_FILTER_MODE:
      return "sampler filter mode";
    case SPV_OPERAND_TYPE_SAMPLER_IMAGE_FORMAT:
      return "image format";
    case SPV_OPERAND_TYPE_FP_FAST_MATH_MODE:
      return "floating-point fast math mode";
    case SPV_OPERAND_TYPE_FP_ROUNDING_MODE:
      return "floating-point rounding mode";
    case SPV_OPERAND_TYPE_LINKAGE_TYPE:
      return "linkage type";
    case SPV_OPERAND_TYPE_ACCESS_QUALIFIER:
    case SPV_OPERAND_TYPE_OPTIONAL_ACCESS_QUALIFIER:
      return "access qualifier";
    case SPV_OPERAND_TYPE_FUNCTION_PARAMETER_ATTRIBUTE:
      return "function parameter attribute";
    case SPV_OPERAND_TYPE_DECORATION:
      return "decoration";
    case SPV_OPERAND_TYPE_BUILT_IN:
      return "built-in";
    case SPV_OPERAND_TYPE_SELECTION_CONTROL:
      return "selection control";
    case SPV_OPERAND_TYPE_LOOP_CONTROL:
      return "loop control";
    case SPV_OPERAND_TYPE_FUNCTION_CONTROL:
      return "function control";
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID:
      return "memory semantics ID";
    case SPV_OPERAND_TYPE_MEMORY_ACCESS:
    case SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS:
      return "memory access";
    case SPV_OPERAND_TYPE_SCOPE_ID:
      return "scope ID";
    case SPV_OPERAND_TYPE_GROUP_OPERATION:
      return "group operation";
    case SPV_OPERAND_TYPE_KERNEL_ENQ_FLAGS:
      return "kernel enqeue flags";
    case SPV_OPERAND_TYPE_KERNEL_PROFILING_INFO:
      return "kernel profiling info";
    case SPV_OPERAND_TYPE_CAPABILITY:
      return "capability";
    case SPV_OPERAND_TYPE_IMAGE:
    case SPV_OPERAND_TYPE_OPTIONAL_IMAGE:
      return "image";
    case SPV_OPERAND_TYPE_OPTIONAL_CIV:
      return "context-insensitive value";

    // The next values are for values returned from an instruction, not actually
    // an operand.  So the specific strings don't matter.  But let's add them
    // for completeness and ease of testing.
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_ORDER:
      return "image channel order";
    case SPV_OPERAND_TYPE_IMAGE_CHANNEL_DATA_TYPE:
      return "image channel data type";

    case SPV_OPERAND_TYPE_NONE:
      return "NONE";
    default:
      assert(0 && "Unhandled operand type!");
      break;
  }
  return "unknown";
}

void spvPrependOperandTypes(const spv_operand_type_t* types,
                            spv_operand_pattern_t* pattern) {
  const spv_operand_type_t* endTypes;
  for (endTypes = types; *endTypes != SPV_OPERAND_TYPE_NONE; ++endTypes)
    ;
  pattern->insert(pattern->begin(), types, endTypes);
}

void spvPrependOperandTypesForMask(const spv_operand_table operandTable,
                                   const spv_operand_type_t type,
                                   const uint32_t mask,
                                   spv_operand_pattern_t* pattern) {
  // Scan from highest bits to lowest bits because we will prepend in LIFO
  // fashion, and we need the operands for lower order bits to appear first.
  for (uint32_t candidate_bit = (1 << 31); candidate_bit; candidate_bit >>= 1) {
    if (candidate_bit & mask) {
      spv_operand_desc entry = nullptr;
      if (SPV_SUCCESS == spvOperandTableValueLookup(operandTable, type,
                                                    candidate_bit, &entry)) {
        spvPrependOperandTypes(entry->operandTypes, pattern);
      }
    }
  }
}

bool spvOperandIsConcreteMask(spv_operand_type_t type) {
  return SPV_OPERAND_TYPE_FIRST_CONCRETE_MASK_TYPE <= type &&
         type <= SPV_OPERAND_TYPE_LAST_CONCRETE_MASK_TYPE;
}

bool spvOperandIsOptional(spv_operand_type_t type) {
  return SPV_OPERAND_TYPE_FIRST_OPTIONAL_TYPE <= type &&
         type <= SPV_OPERAND_TYPE_LAST_OPTIONAL_TYPE;
}

bool spvOperandIsVariable(spv_operand_type_t type) {
  return SPV_OPERAND_TYPE_FIRST_VARIABLE_TYPE <= type &&
         type <= SPV_OPERAND_TYPE_LAST_VARIABLE_TYPE;
}

bool spvExpandOperandSequenceOnce(spv_operand_type_t type,
                                  spv_operand_pattern_t* pattern) {
  switch (type) {
    case SPV_OPERAND_TYPE_VARIABLE_ID:
      pattern->insert(pattern->begin(), {SPV_OPERAND_TYPE_OPTIONAL_ID, type});
      return true;
    case SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER:
      pattern->insert(pattern->begin(),
                      {SPV_OPERAND_TYPE_OPTIONAL_LITERAL_INTEGER, type});
      return true;
    case SPV_OPERAND_TYPE_VARIABLE_LITERAL_INTEGER_ID:
      // Represents Zero or more (Literal number, Id) pairs,
      // where the literal number must be a scalar integer.
      pattern->insert(pattern->begin(),
                      {SPV_OPERAND_TYPE_OPTIONAL_TYPED_LITERAL_INTEGER,
                       SPV_OPERAND_TYPE_ID, type});
      return true;
    case SPV_OPERAND_TYPE_VARIABLE_ID_LITERAL_INTEGER:
      // Represents Zero or more (Id, Literal number) pairs.
      pattern->insert(pattern->begin(),
                      {SPV_OPERAND_TYPE_OPTIONAL_ID,
                       SPV_OPERAND_TYPE_LITERAL_INTEGER, type});
      return true;
    default:
      break;
  }
  return false;
}

spv_operand_type_t spvTakeFirstMatchableOperand(
    spv_operand_pattern_t* pattern) {
  assert(!pattern->empty());
  spv_operand_type_t result;
  do {
    result = pattern->front();
    pattern->pop_front();
  } while (spvExpandOperandSequenceOnce(result, pattern));
  return result;
}

spv_operand_pattern_t spvAlternatePatternFollowingImmediate(
    const spv_operand_pattern_t& pattern) {
  spv_operand_pattern_t alternatePattern;
  for (const auto& operand : pattern) {
    if (operand == SPV_OPERAND_TYPE_RESULT_ID) {
      alternatePattern.push_back(operand);
      alternatePattern.push_back(SPV_OPERAND_TYPE_OPTIONAL_CIV);
      return alternatePattern;
    }
    alternatePattern.push_back(SPV_OPERAND_TYPE_OPTIONAL_CIV);
  }
  // No result-id found, so just expect CIVs.
  return {SPV_OPERAND_TYPE_OPTIONAL_CIV};
}

bool spvIsIdType(spv_operand_type_t type) {
  switch (type) {
    case SPV_OPERAND_TYPE_ID:
    case SPV_OPERAND_TYPE_TYPE_ID:
    case SPV_OPERAND_TYPE_RESULT_ID:
    case SPV_OPERAND_TYPE_MEMORY_SEMANTICS_ID:
    case SPV_OPERAND_TYPE_SCOPE_ID:
      return true;
    default:
      return false;
  }
  return false;
}
