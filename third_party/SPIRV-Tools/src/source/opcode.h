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

#ifndef LIBSPIRV_OPCODE_H_
#define LIBSPIRV_OPCODE_H_

#include "instruction.h"
#include "spirv-tools/libspirv.h"
#include "spirv/spirv.h"
#include "table.h"

// Returns the name of a registered SPIR-V generator as a null-terminated
// string. If the generator is not known, then returns the string "Unknown".
// The generator parameter should be most significant 16-bits of the generator
// word in the SPIR-V module header.
//
// See the registry at https://www.khronos.org/registry/spir-v/api/spir-v.xml.
const char* spvGeneratorStr(uint32_t generator);

// Combines word_count and opcode enumerant in single word.
uint32_t spvOpcodeMake(uint16_t word_count, SpvOp opcode);

// Splits word into into two constituent parts: word_count and opcode.
void spvOpcodeSplit(const uint32_t word, uint16_t* word_count,
                    uint16_t* opcode);

// Finds the named opcode in the given opcode table. On success, returns
// SPV_SUCCESS and writes a handle of the table entry into *entry.
spv_result_t spvOpcodeTableNameLookup(const spv_opcode_table table,
                                      const char* name, spv_opcode_desc* entry);

// Finds the opcode by enumerant in the given opcode table. On success, returns
// SPV_SUCCESS and writes a handle of the table entry into *entry.
spv_result_t spvOpcodeTableValueLookup(const spv_opcode_table table,
                                       const SpvOp opcode,
                                       spv_opcode_desc* entry);

// Determines if the opcode has capability requirements. Returns zero if false,
// non-zero otherwise. This function does not check if the given entry is valid.
int32_t spvOpcodeRequiresCapabilities(spv_opcode_desc opcode);

// Copies an instruction's word and fixes the endianness to host native. The
// source instruction's stream/opcode/endianness is in the words/opcode/endian
// parameter. The word_count parameter specifies the number of words to copy.
// Writes copied instruction into *inst.
void spvInstructionCopy(const uint32_t* words, const SpvOp opcode,
                        const uint16_t word_count,
                        const spv_endianness_t endian, spv_instruction_t* inst);

// Gets the name of an instruction, without the "Op" prefix.
const char* spvOpcodeString(const SpvOp opcode);

// Determine if the given opcode is a scalar type. Returns zero if false,
// non-zero otherwise.
int32_t spvOpcodeIsScalarType(const SpvOp opcode);

// Determines if the given opcode is a constant. Returns zero if false, non-zero
// otherwise.
int32_t spvOpcodeIsConstant(const SpvOp opcode);

// Determines if the given opcode is a composite type. Returns zero if false,
// non-zero otherwise.
int32_t spvOpcodeIsComposite(const SpvOp opcode);

// Determines if the given opcode results in a pointer. Returns zero if false,
// non-zero otherwise.
int32_t spvOpcodeIsPointer(const SpvOp opcode);

// Determines if the given opcode generates a type. Returns zero if false,
// non-zero otherwise.
int32_t spvOpcodeGeneratesType(SpvOp opcode);

#endif  // LIBSPIRV_OPCODE_H_
