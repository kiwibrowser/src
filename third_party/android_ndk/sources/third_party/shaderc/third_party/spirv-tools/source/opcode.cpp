// Copyright (c) 2015-2016 The Khronos Group Inc.
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

#include "opcode.h"

#include <assert.h>
#include <string.h>

#include <algorithm>
#include <cstdlib>

#include "instruction.h"
#include "macro.h"
#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"
#include "spirv_endian.h"

namespace {

// Descriptions of each opcode.  Each entry describes the format of the
// instruction that follows a particular opcode.
const spv_opcode_desc_t opcodeTableEntries_1_0[] = {
#include "core.insts-1.0.inc"
};
const spv_opcode_desc_t opcodeTableEntries_1_1[] = {
#include "core.insts-1.1.inc"
};
const spv_opcode_desc_t opcodeTableEntries_1_2[] = {
#include "core.insts-1.2.inc"
};

// Represents a vendor tool entry in the SPIR-V XML Regsitry.
struct VendorTool {
  uint32_t value;
  const char* vendor;
  const char* tool; // Might be empty string.
  const char* vendor_tool; // Combiantion of vendor and tool.
};

const VendorTool vendor_tools[] = {
#include "generators.inc"
};

}  // anonymous namespace

// TODO(dneto): Move this to another file.  It doesn't belong with opcode
// processing.
const char* spvGeneratorStr(uint32_t generator) {
  auto where = std::find_if(
      std::begin(vendor_tools), std::end(vendor_tools),
      [generator](const VendorTool& vt) { return generator == vt.value; });
  if (where != std::end(vendor_tools)) return where->vendor_tool;
  return "Unknown";
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

spv_result_t spvOpcodeTableGet(spv_opcode_table* pInstTable,
                               spv_target_env env) {
  if (!pInstTable) return SPV_ERROR_INVALID_POINTER;

  static const spv_opcode_table_t table_1_0 = {
      ARRAY_SIZE(opcodeTableEntries_1_0), opcodeTableEntries_1_0};
  static const spv_opcode_table_t table_1_1 = {
      ARRAY_SIZE(opcodeTableEntries_1_1), opcodeTableEntries_1_1};
  static const spv_opcode_table_t table_1_2 = {
      ARRAY_SIZE(opcodeTableEntries_1_2), opcodeTableEntries_1_2};

  switch (env) {
    case SPV_ENV_UNIVERSAL_1_0:
    case SPV_ENV_VULKAN_1_0:
    case SPV_ENV_OPENCL_2_1:
    case SPV_ENV_OPENGL_4_0:
    case SPV_ENV_OPENGL_4_1:
    case SPV_ENV_OPENGL_4_2:
    case SPV_ENV_OPENGL_4_3:
    case SPV_ENV_OPENGL_4_5:
      *pInstTable = &table_1_0;
      return SPV_SUCCESS;
    case SPV_ENV_UNIVERSAL_1_1:
      *pInstTable = &table_1_1;
      return SPV_SUCCESS;
    case SPV_ENV_UNIVERSAL_1_2:
    case SPV_ENV_OPENCL_2_2:
      *pInstTable = &table_1_2;
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
  for (uint32_t i = 0; i < ARRAY_SIZE(opcodeTableEntries_1_2); ++i) {
    if (opcodeTableEntries_1_2[i].opcode == opcode)
      return opcodeTableEntries_1_2[i].name;
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

bool spvOpcodeIsConstantOrUndef(const SpvOp opcode) {
  return opcode == SpvOpUndef || spvOpcodeIsConstant(opcode);
}

bool spvOpcodeIsScalarSpecConstant(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpSpecConstantTrue:
    case SpvOpSpecConstantFalse:
    case SpvOpSpecConstant:
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

bool spvOpcodeReturnsLogicalVariablePointer(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpVariable:
    case SpvOpAccessChain:
    case SpvOpInBoundsAccessChain:
    case SpvOpFunctionParameter:
    case SpvOpImageTexelPointer:
    case SpvOpCopyObject:
    case SpvOpSelect:
    case SpvOpPhi:
    case SpvOpFunctionCall:
    case SpvOpPtrAccessChain:
    case SpvOpLoad:
    case SpvOpConstantNull:
      return true;
    default:
      return false;
  }
}

int32_t spvOpcodeReturnsLogicalPointer(const SpvOp opcode) {
  switch (opcode) {
    case SpvOpVariable:
    case SpvOpAccessChain:
    case SpvOpInBoundsAccessChain:
    case SpvOpFunctionParameter:
    case SpvOpImageTexelPointer:
    case SpvOpCopyObject:
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
    case SpvOpTypePipeStorage:
    case SpvOpTypeNamedBarrier:
      return true;
    default:
      // In particular, OpTypeForwardPointer does not generate a type,
      // but declares a storage class for a pointer type generated
      // by a different instruction.
      break;
  }
  return 0;
}
