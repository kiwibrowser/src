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

#ifndef LIBSPIRV_INSTRUCTION_H_
#define LIBSPIRV_INSTRUCTION_H_

#include <cstdint>
#include <vector>

#include "spirv/spirv.h"

#include "table.h"

// Describes an instruction.
struct spv_instruction_t {
  // Normally, both opcode and extInstType contain valid data.
  // However, when the assembler parses !<number> as the first word in
  // an instruction and opcode and extInstType are invalid.
  SpvOp opcode;
  spv_ext_inst_type_t extInstType;

  // The Id of the result type, if this instruction has one.  Zero otherwise.
  uint32_t resultTypeId;

  // The instruction, as a sequence of 32-bit words.
  // For a regular instruction the opcode and word count are combined
  // in words[0], as described in the SPIR-V spec.
  // Otherwise, the first token was !<number>, and that number appears
  // in words[0].  Subsequent elements are the result of parsing
  // tokens in the alternate parsing mode as described in syntax.md.
  std::vector<uint32_t> words;
};

// Appends a word to an instruction, without checking for overflow.
void spvInstructionAddWord(spv_instruction_t* inst, uint32_t value);

#endif  // LIBSPIRV_INSTRUCTION_H_
