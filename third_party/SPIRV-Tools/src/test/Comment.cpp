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

#include "gmock/gmock.h"
#include "TestFixture.h"
#include "UnitSPIRV.h"

namespace {

using spvtest::Concatenate;
using spvtest::MakeInstruction;
using spvtest::MakeVector;
using spvtest::TextToBinaryTest;
using testing::Eq;

TEST_F(TextToBinaryTest, Whitespace) {
  std::string input = R"(
; I'm a proud comment at the beginning of the file
; I hide:   OpCapability Shader
            OpMemoryModel Logical Simple ; comment after instruction
;;;;;;;; many ;'s
 %glsl450 = OpExtInstImport "GLSL.std.450"
            ; comment indented
)";

  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(Concatenate({MakeInstruction(SpvOpMemoryModel,
                                      {uint32_t(SpvAddressingModelLogical),
                                       uint32_t(SpvMemoryModelSimple)}),
                      MakeInstruction(SpvOpExtInstImport, {1},
                                      MakeVector("GLSL.std.450"))})));
}

}  // anonymous namespace
