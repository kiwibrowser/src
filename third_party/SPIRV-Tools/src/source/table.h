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

#ifndef LIBSPIRV_TABLE_H_
#define LIBSPIRV_TABLE_H_

#include "spirv-tools/libspirv.h"
#include "spirv/spirv.h"
#include "spirv_definition.h"

typedef struct spv_opcode_desc_t {
  const char* name;
  const SpvOp opcode;
  const spv_capability_mask_t
      capabilities;  // Bitfield of SPV_CAPABILITY_AS_MASK(spv::Capability)
  // operandTypes[0..numTypes-1] describe logical operands for the instruction.
  // The operand types include result id and result-type id, followed by
  // the types of arguments.
  uint16_t numTypes;
  spv_operand_type_t operandTypes[16];  // TODO: Smaller/larger?
  const bool hasResult;  // Does the instruction have a result ID operand?
  const bool hasType;    // Does the instruction have a type ID operand?
} spv_opcode_desc_t;

typedef struct spv_operand_desc_t {
  const char* name;
  const uint32_t value;
  const spv_capability_mask_t
      capabilities;  // Bitfield of SPV_CAPABILITY_AS_MASK(spv::Capability)
  const spv_operand_type_t operandTypes[16];  // TODO: Smaller/larger?
} spv_operand_desc_t;

typedef struct spv_operand_desc_group_t {
  const spv_operand_type_t type;
  const uint32_t count;
  const spv_operand_desc_t* entries;
} spv_operand_desc_group_t;

typedef struct spv_ext_inst_desc_t {
  const char* name;
  const uint32_t ext_inst;
  const spv_capability_mask_t
      capabilities;  // Bitfield of SPV_CAPABILITY_AS_MASK(spv::Capability)
  const spv_operand_type_t operandTypes[16];  // TODO: Smaller/larger?
} spv_ext_inst_desc_t;

typedef struct spv_ext_inst_group_t {
  const spv_ext_inst_type_t type;
  const uint32_t count;
  const spv_ext_inst_desc_t* entries;
} spv_ext_inst_group_t;

typedef struct spv_opcode_table_t {
  const uint32_t count;
  const spv_opcode_desc_t* entries;
} spv_opcode_table_t;

typedef struct spv_operand_table_t {
  const uint32_t count;
  const spv_operand_desc_group_t* types;
} spv_operand_table_t;

typedef struct spv_ext_inst_table_t {
  const uint32_t count;
  const spv_ext_inst_group_t* groups;
} spv_ext_inst_table_t;

typedef const spv_opcode_desc_t* spv_opcode_desc;
typedef const spv_operand_desc_t* spv_operand_desc;
typedef const spv_ext_inst_desc_t* spv_ext_inst_desc;

typedef const spv_opcode_table_t* spv_opcode_table;
typedef const spv_operand_table_t* spv_operand_table;
typedef const spv_ext_inst_table_t* spv_ext_inst_table;

struct spv_context_t {
  const spv_target_env target_env;
  const spv_opcode_table opcode_table;
  const spv_operand_table operand_table;
  const spv_ext_inst_table ext_inst_table;
};

// Populates *table with entries for env.
spv_result_t spvOpcodeTableGet(spv_opcode_table* table, spv_target_env env);

// Populates *table with entries for env.
spv_result_t spvOperandTableGet(spv_operand_table* table, spv_target_env env);

// Populates *table with entries for env.
spv_result_t spvExtInstTableGet(spv_ext_inst_table* table, spv_target_env env);

#endif  // LIBSPIRV_TABLE_H_
