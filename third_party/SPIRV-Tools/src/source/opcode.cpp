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

#include "opcode.h"

#include <assert.h>
#include <string.h>

#include <cstdlib>

#include "instruction.h"
#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"
#include "spirv_endian.h"

namespace {

// Descriptions of each opcode.  Each entry describes the format of the
// instruction that follows a particular opcode.
const spv_opcode_desc_t opcodeTableEntries_1_0[] = {
#include "core.insts-1-0.inc"
};
const spv_opcode_desc_t opcodeTableEntries_1_1[] = {
#include "core.insts-1-1.inc"
};

}  // anonymous namespace

const char* spvGeneratorStr(uint32_t generator) {
  switch (generator) {
    case SPV_GENERATOR_KHRONOS:
      return "Khronos";
    case SPV_GENERATOR_LUNARG:
      return "LunarG";
    case SPV_GENERATOR_VALVE:
      return "Valve";
    case SPV_GENERATOR_CODEPLAY:
      return "Codeplay Software Ltd.";
    case SPV_GENERATOR_NVIDIA:
      return "NVIDIA";
    case SPV_GENERATOR_ARM:
      return "ARM";
    case SPV_GENERATOR_KHRONOS_LLVM_TRANSLATOR:
      return "Khronos LLVM/SPIR-V Translator";
    case SPV_GENERATOR_KHRONOS_ASSEMBLER:
      return "Khronos SPIR-V Tools Assembler";
    case SPV_GENERATOR_KHRONOS_GLSLANG:
      return "Khronos Glslang Reference Front End";
    default:
      return "Unknown";
  }
}

uint32_t spvOpcodeMake(uint16_t wordCount, SpvOp opcode) {
  return ((uint32_t)opcode) | (((uint32_t)wordCount) << 16);
}

void spvOpcodeSplit(const uint32_t word, uint16_t* pWordCount,
                    uint16_t* pOpcode) {
  if (pWordCount) {
    *pWordCount = (uint16_t)((0xffff0000 & word) >> 16);
  }
  if (pOpcode) {
    *pOpcode = 0x0000ffff & word;
  }
}

// Evaluates to the number of elements of array A.
// If we could use constexpr, then we could make this a template function.
// If the source arrays were std::array, then we could have used
// std::array::size.
#define ARRAY_SIZE(A) (static_cast<uint32_t>(sizeof(A) / sizeof(A[0])))

spv_result_t spvOpcodeTableGet(spv_opcode_table* pInstTable,
                               spv_target_env env) {
  if (!pInstTable) return SPV_ERROR_INVALID_POINTER;

  static const spv_opcode_table_t table_1_0 = {
      ARRAY_SIZE(opcodeTableEntries_1_0), opcodeTableEntries_1_0};
  static const spv_opcode_table_t table_1_1 = {
      ARRAY_SIZE(opcodeTableEntries_1_1), opcodeTableEntries_1_1};

  switch (env) {
    case SPV_ENV_UNIVERSAL_1_0:
    case SPV_ENV_VULKAN_1_0:
      *pInstTable = &table_1_0;
      return SPV_SUCCESS;
    case SPV_ENV_UNIVERSAL_1_1:
      *pInstTable = &table_1_1;
      return SPV_SUCCESS;
  }
  assert(0 && "Unknown spv_target_env in spvOpcodeTableGet()");
  return SPV_ERROR_INVALID_TABLE;
}

spv_result_t spvOpcodeTableNameLookup(const spv_opcode_table table,
                                      const char* name,
                                      spv_opcode_desc* pEntry) {
  if (!name || !pEntry) return SPV_ERROR_INVALID_POINTER;
  if (!table) return SPV_ERROR_INVALID_TABLE;

  // TODO: This lookup of the Opcode table is suboptimal! Binary sort would be
  // preferable but the table requires sorting on the Opcode name, but it's
  // static
  // const initialized and matches the order of the spec.
  const size_t nameLength = strlen(name);
  for (uint64_t opcodeIndex = 0; opcodeIndex < table->count; ++opcodeIndex) {
    if (nameLength == strlen(table->entries[opcodeIndex].name) &&
        !strncmp(name, table->entries[opcodeIndex].name, nameLength)) {
      // NOTE: Found out Opcode!
      *pEntry = &table->entries[opcodeIndex];
      return SPV_SUCCESS;
    }
  }

  return SPV_ERROR_INVALID_LOOKUP;
}

spv_result_t spvOpcodeTableValueLookup(const spv_opcode_table table,
                                       const SpvOp opcode,
                                       spv_opcode_desc* pEntry) {
  if (!table) return SPV_ERROR_INVALID_TABLE;
  if (!pEntry) return SPV_ERROR_INVALID_POINTER;

  // TODO: As above this lookup is not optimal.
  for (uint64_t opcodeIndex = 0; opcodeIndex < table->count; ++opcodeIndex) {
    if (opcode == table->entries[opcodeIndex].opcode) {
      // NOTE: Found the Opcode!
      *pEntry = &table->entries[opcodeIndex];
      return SPV_SUCCESS;
    }
  }

  return SPV_ERROR_INVALID_LOOKUP;
}

int32_t spvOpcodeRequiresCapabilities(spv_opcode_desc entry) {
  return entry->capabilities != 0;
}

void spvInstructionCopy(const uint32_t* words, const SpvOp opcode,
                        const uint16_t wordCount, const spv_endianness_t endian,
                        spv_instruction_t* pInst) {
  pInst->opcode = opcode;
  pInst->words.resize(wordCount);
  for (uint16_t wordIndex = 0; wordIndex < wordCount; ++wordIndex) {
    pInst->words[wordIndex] = spvFixWord(words[wordIndex], endian);
    if (!wordIndex) {
      uint16_t thisWordCount;
      uint16_t thisOpcode;
      spvOpcodeSplit(pInst->words[wordIndex], &thisWordCount, &thisOpcode);
      assert(opcode == static_cast<SpvOp>(thisOpcode) &&
             wordCount == thisWordCount && "Endianness failed!");
    }
  }
}

const char* spvOpcodeString(const SpvOp opcode) {
  // Use the latest SPIR-V version, which should be backward-compatible with all
  // previous ones.
  for (uint32_t i = 0;
       i < sizeof(opcodeTableEntries_1_1) / sizeof(spv_opcode_desc_t); ++i) {
    if (opcodeTableEntries_1_1[i].opcode == opcode)
      return opcodeTableEntries_1_1[i].name;
  }
  assert(0 && "Unreachable!");
  return "unknown";
}

int32_t spvOpcodeIsScalarType(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpTypeInt:
    case SpvOpTypeFloat:
    case SpvOpTypeBool:
      return true;
    default:
      return false;
  }
}

int32_t spvOpcodeIsConstant(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpConstantTrue:
    case SpvOpConstantFalse:
    case SpvOpConstant:
    case SpvOpConstantComposite:
    case SpvOpConstantSampler:
    case SpvOpConstantNull:
    case SpvOpSpecConstantTrue:
    case SpvOpSpecConstantFalse:
    case SpvOpSpecConstant:
    case SpvOpSpecConstantComposite:
    case SpvOpSpecConstantOp:
      return true;
    default:
      return false;
  }
}

int32_t spvOpcodeIsComposite(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpTypeVector:
    case SpvOpTypeMatrix:
    case SpvOpTypeArray:
    case SpvOpTypeStruct:
      return true;
    default:
      return false;
  }
}

int32_t spvOpcodeIsPointer(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpVariable:
    case SpvOpAccessChain:
    case SpvOpPtrAccessChain:
    case SpvOpInBoundsAccessChain:
    case SpvOpInBoundsPtrAccessChain:
    case SpvOpFunctionParameter:
      return true;
    default:
      return false;
  }
}

int32_t spvOpcodeGeneratesType(SpvOp op) {
  switch (op) {
    case SpvOpTypeVoid:
    case SpvOpTypeBool:
    case SpvOpTypeInt:
    case SpvOpTypeFloat:
    case SpvOpTypeVector:
    case SpvOpTypeMatrix:
    case SpvOpTypeImage:
    case SpvOpTypeSampler:
    case SpvOpTypeSampledImage:
    case SpvOpTypeArray:
    case SpvOpTypeRuntimeArray:
    case SpvOpTypeStruct:
    case SpvOpTypeOpaque:
    case SpvOpTypePointer:
    case SpvOpTypeFunction:
    case SpvOpTypeEvent:
    case SpvOpTypeDeviceEvent:
    case SpvOpTypeReserveId:
    case SpvOpTypeQueue:
    case SpvOpTypePipe:
      return true;
    default:
      // In particular, OpTypeForwardPointer does not generate a type,
      // but declares a storage class for a pointer type generated
      // by a different instruction.
      break;
  }
  return 0;
}
