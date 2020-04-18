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

#include "UnitSPIRV.h"

#include <gmock/gmock.h>

#include "source/opcode.h"

using ::spvtest::EnumCase;
using ::testing::Eq;

namespace {

using GeneratorMagicNumberTest =
    ::testing::TestWithParam<EnumCase<spv_generator_t>>;

TEST_P(GeneratorMagicNumberTest, Single) {
  EXPECT_THAT(std::string(spvGeneratorStr(GetParam().value())),
              GetParam().name());
}

INSTANTIATE_TEST_CASE_P(
    Registered, GeneratorMagicNumberTest,
    ::testing::ValuesIn(std::vector<EnumCase<spv_generator_t>>{
        {SPV_GENERATOR_KHRONOS, "Khronos"},
        {SPV_GENERATOR_LUNARG, "LunarG"},
        {SPV_GENERATOR_VALVE, "Valve"},
        {SPV_GENERATOR_CODEPLAY, "Codeplay Software Ltd."},
        {SPV_GENERATOR_NVIDIA, "NVIDIA"},
        {SPV_GENERATOR_ARM, "ARM"},
        {SPV_GENERATOR_KHRONOS_LLVM_TRANSLATOR,
         "Khronos LLVM/SPIR-V Translator"},
        {SPV_GENERATOR_KHRONOS_ASSEMBLER, "Khronos SPIR-V Tools Assembler"},
        {SPV_GENERATOR_KHRONOS_GLSLANG, "Khronos Glslang Reference Front End"},
    }),);

INSTANTIATE_TEST_CASE_P(
    Unregistered, GeneratorMagicNumberTest,
    ::testing::ValuesIn(std::vector<EnumCase<spv_generator_t>>{
        // Currently value 6 and beyond are unregiestered.
        {spv_generator_t(9), "Unknown"},
        {spv_generator_t(9999), "Unknown"},
    }),);
}  // anonymous namespace
