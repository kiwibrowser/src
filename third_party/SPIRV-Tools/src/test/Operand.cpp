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

namespace {

using GetTargetTest = ::testing::TestWithParam<spv_target_env>;
using ::testing::ValuesIn;
using std::vector;

TEST_P(GetTargetTest, Default) {
  spv_operand_table table;
  ASSERT_EQ(SPV_SUCCESS, spvOperandTableGet(&table, GetParam()));
  ASSERT_NE(0u, table->count);
  ASSERT_NE(nullptr, table->types);
}

TEST_P(GetTargetTest, InvalidPointerTable) {
  ASSERT_EQ(SPV_ERROR_INVALID_POINTER, spvOperandTableGet(nullptr, GetParam()));
}

INSTANTIATE_TEST_CASE_P(OperandTableGet, GetTargetTest,
                        ValuesIn(vector<spv_target_env>{SPV_ENV_UNIVERSAL_1_0,
                                                        SPV_ENV_UNIVERSAL_1_1,
                                                        SPV_ENV_VULKAN_1_0}), );

TEST(OperandString, AllAreDefinedExceptVariable) {
  // None has no string, so don't test it.
  EXPECT_EQ(0u, SPV_OPERAND_TYPE_NONE);
  // Start testing at enum with value 1, skipping None.
  for (int i = 1; i < int(SPV_OPERAND_TYPE_FIRST_VARIABLE_TYPE); i++) {
    EXPECT_NE(nullptr, spvOperandTypeStr(static_cast<spv_operand_type_t>(i)))
        << " Operand type " << i;
  }
}

TEST(OperandIsConcreteMask, Sample) {
  // Check a few operand types preceding the concrete mask types.
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_NONE));
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_ID));
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_LITERAL_INTEGER));
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_CAPABILITY));

  // Check all the concrete mask operand types.
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_IMAGE));
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_FP_FAST_MATH_MODE));
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_SELECTION_CONTROL));
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_LOOP_CONTROL));
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_FUNCTION_CONTROL));
  EXPECT_TRUE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_MEMORY_ACCESS));

  // Check a few operand types after the concrete mask types, including the
  // optional forms for Image and MemoryAccess.
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_OPTIONAL_ID));
  EXPECT_FALSE(spvOperandIsConcreteMask(SPV_OPERAND_TYPE_OPTIONAL_IMAGE));
  EXPECT_FALSE(
      spvOperandIsConcreteMask(SPV_OPERAND_TYPE_OPTIONAL_MEMORY_ACCESS));
}

}  // anonymous namespace
