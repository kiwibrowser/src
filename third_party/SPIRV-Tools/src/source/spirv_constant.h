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

#ifndef LIBSPIRV_SPIRV_CONSTANT_H_
#define LIBSPIRV_SPIRV_CONSTANT_H_

#include "spirv-tools/libspirv.h"
#include "spirv/spirv.h"

// Version number macros.

// Evaluates to a well-formed version header word, given valid
// SPIR-V version major and minor version numbers.
#define SPV_SPIRV_VERSION_WORD(MAJOR, MINOR) \
  ((uint32_t(uint8_t(MAJOR)) << 16) | (uint32_t(uint8_t(MINOR)) << 8))
// Returns the major version extracted from a version header word.
#define SPV_SPIRV_VERSION_MAJOR_PART(WORD) ((uint32_t(WORD) >> 16) & 0xff)
// Returns the minor version extracted from a version header word.
#define SPV_SPIRV_VERSION_MINOR_PART(WORD) ((uint32_t(WORD) >> 8) & 0xff)

// Returns the version number for the given SPIR-V target environment.
uint32_t spvVersionForTargetEnv(spv_target_env env);

// Header indices

#define SPV_INDEX_MAGIC_NUMBER 0u
#define SPV_INDEX_VERSION_NUMBER 1u
#define SPV_INDEX_GENERATOR_NUMBER 2u
#define SPV_INDEX_BOUND 3u
#define SPV_INDEX_SCHEMA 4u
#define SPV_INDEX_INSTRUCTION 5u

// Universal limits

// SPIR-V 1.0 limits
#define SPV_LIMIT_INSTRUCTION_WORD_COUNT_MAX 0xffff
#define SPV_LIMIT_LITERAL_STRING_UTF8_CHARS_MAX 0xffff

// A single Unicode character in UTF-8 encoding can take
// up 4 bytes.
#define SPV_LIMIT_LITERAL_STRING_BYTES_MAX \
  (SPV_LIMIT_LITERAL_STRING_UTF8_CHARS_MAX * 4)

// NOTE: These are set to the minimum maximum values
// TODO(dneto): Check these.

// libspirv limits.
#define SPV_LIMIT_RESULT_ID_BOUND 0x00400000
#define SPV_LIMIT_CONTROL_FLOW_NEST_DEPTH 0x00000400
#define SPV_LIMIT_GLOBAL_VARIABLES_MAX 0x00010000
#define SPV_LIMIT_LOCAL_VARIABLES_MAX 0x00080000
// TODO: Decorations per target ID max, depends on decoration table size
#define SPV_LIMIT_EXECUTION_MODE_PER_ENTRY_POINT_MAX 0x00000100
#define SPV_LIMIT_INDICIES_MAX_ACCESS_CHAIN_COMPOSITE_MAX 0x00000100
#define SPV_LIMIT_FUNCTION_PARAMETERS_PER_FUNCTION_DECL 0x00000100
#define SPV_LIMIT_FUNCTION_CALL_ARGUMENTS_MAX 0x00000100
#define SPV_LIMIT_EXT_FUNCTION_CALL_ARGUMENTS_MAX 0x00000100
#define SPV_LIMIT_SWITCH_LITERAL_LABEL_PAIRS_MAX 0x00004000
#define SPV_LIMIT_STRUCT_MEMBERS_MAX 0x0000400
#define SPV_LIMIT_STRUCT_NESTING_DEPTH_MAX 0x00000100

// Enumerations

// Values mapping to registered tools.  See the registry at
// https://www.khronos.org/registry/spir-v/api/spir-v.xml
// These values occupy the higher order 16 bits of the generator magic word.
typedef enum spv_generator_t {
  // TODO(dneto) Values 0 through 5 were registered only as vendor.
  SPV_GENERATOR_KHRONOS = 0,
  SPV_GENERATOR_LUNARG = 1,
  SPV_GENERATOR_VALVE = 2,
  SPV_GENERATOR_CODEPLAY = 3,
  SPV_GENERATOR_NVIDIA = 4,
  SPV_GENERATOR_ARM = 5,
  // These are vendor and tool.
  SPV_GENERATOR_KHRONOS_LLVM_TRANSLATOR = 6,
  SPV_GENERATOR_KHRONOS_ASSEMBLER = 7,
  SPV_GENERATOR_KHRONOS_GLSLANG = 8,
  SPV_GENERATOR_NUM_ENTRIES,
  SPV_FORCE_16_BIT_ENUM(spv_generator_t)
} spv_generator_t;

// Evaluates to a well-formed generator magic word from a tool value and
// miscellaneous 16-bit value.
#define SPV_GENERATOR_WORD(TOOL, MISC) \
  ((uint32_t(uint16_t(TOOL)) << 16) | uint16_t(MISC))
// Returns the tool component of the generator word.
#define SPV_GENERATOR_TOOL_PART(WORD) (uint32_t(WORD) >> 16)
// Returns the misc part of the generator word.
#define SPV_GENERATOR_MISC_PART(WORD) (uint32_t(WORD) & 0xFFFF)

#endif  // LIBSPIRV_SPIRV_CONSTANT_H_
