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

// Validation tests for OpVariable storage class

#include <sstream>
#include <string>
#include <tuple>

#include "ValidateFixtures.h"
#include "gmock/gmock.h"

using ValidateStorage = spvtest::ValidateBase<std::string>;

namespace {

TEST_F(ValidateStorage, FunctionStorageInsideFunction) {
  char str[] = R"(
          OpCapability Shader
          OpMemoryModel Logical GLSL450
%intt   = OpTypeInt 32 1
%voidt  = OpTypeVoid
%vfunct = OpTypeFunction %voidt
%ptrt   = OpTypePointer Function %intt
%func   = OpFunction %voidt None %vfunct
%funcl  = OpLabel
%var    = OpVariable %ptrt Function
          OpReturn
          OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

TEST_F(ValidateStorage, FunctionStorageOutsideFunction) {
  char str[] = R"(
          OpCapability Shader
          OpMemoryModel Logical GLSL450
%intt   = OpTypeInt 32 1
%voidt  = OpTypeVoid
%vfunct = OpTypeFunction %voidt
%ptrt   = OpTypePointer Function %intt
%var    = OpVariable %ptrt Function
%func   = OpFunction %voidt None %vfunct
%funcl  = OpLabel
          OpReturn
          OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

TEST_F(ValidateStorage, OtherStorageOutsideFunction) {
  char str[] = R"(
          OpCapability Shader
          OpCapability Kernel
          OpCapability AtomicStorage
          OpMemoryModel Logical GLSL450
%intt   = OpTypeInt 32 1
%voidt  = OpTypeVoid
%vfunct = OpTypeFunction %voidt
%ptrt   = OpTypePointer Function %intt
%unicon = OpVariable %ptrt UniformConstant
%input  = OpVariable %ptrt Input
%unif   = OpVariable %ptrt Uniform
%output = OpVariable %ptrt Output
%wgroup = OpVariable %ptrt Workgroup
%xwgrp  = OpVariable %ptrt CrossWorkgroup
%priv   = OpVariable %ptrt Private
%pushco = OpVariable %ptrt PushConstant
%atomct = OpVariable %ptrt AtomicCounter
%image  = OpVariable %ptrt Image
%func   = OpFunction %voidt None %vfunct
%funcl  = OpLabel
          OpReturn
          OpFunctionEnd
)";

  CompileSuccessfully(str);
  ASSERT_EQ(SPV_SUCCESS, ValidateInstructions());
}

// clang-format off
TEST_P(ValidateStorage, OtherStorageInsideFunction) {
  std::stringstream ss;
  ss << R"(
          OpCapability Shader
          OpCapability Kernel
          OpCapability AtomicStorage
          OpMemoryModel Logical GLSL450
%intt   = OpTypeInt 32 1
%voidt  = OpTypeVoid
%vfunct = OpTypeFunction %voidt
%ptrt   = OpTypePointer Function %intt
%func   = OpFunction %voidt None %vfunct
%funcl  = OpLabel
%var    = OpVariable %ptrt )" << GetParam() << R"(
          OpReturn
          OpFunctionEnd
)";

  CompileSuccessfully(ss.str());
  ASSERT_EQ(SPV_ERROR_INVALID_LAYOUT, ValidateInstructions());
}

INSTANTIATE_TEST_CASE_P(MatrixOp, ValidateStorage,
                        ::testing::Values(
                             "Input",
                             "Uniform",
                             "Output",
                             "Workgroup",
                             "CrossWorkgroup",
                             "Private",
                             "PushConstant",
                             "AtomicCounter",
                             "Image"),);
// clang-format on

TEST_F(ValidateStorage, GenericVariableOutsideFunction) {
  const auto str = R"(
          OpCapability Kernel
          OpMemoryModel Logical OpenCL
%intt   = OpTypeInt 32 1
%ptrt   = OpTypePointer Function %intt
%var    = OpVariable %ptrt Generic
)";
  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_BINARY, ValidateInstructions());
}

TEST_F(ValidateStorage, GenericVariableInsideFunction) {
  const auto str = R"(
          OpCapability Shader
          OpMemoryModel Logical GLSL450
%intt   = OpTypeInt 32 1
%voidt  = OpTypeVoid
%vfunct = OpTypeFunction %voidt
%ptrt   = OpTypePointer Function %intt
%func   = OpFunction %voidt None %vfunct
%funcl  = OpLabel
%var    = OpVariable %ptrt Generic
          OpReturn
          OpFunctionEnd
)";
  CompileSuccessfully(str);
  ASSERT_EQ(SPV_ERROR_INVALID_BINARY, ValidateInstructions());
}
}
