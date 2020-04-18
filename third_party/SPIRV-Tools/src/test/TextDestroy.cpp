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

TEST(TextDestroy, DestroyNull) { spvBinaryDestroy(nullptr); }

TEST(TextDestroy, Default) {
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);
  char textStr[] = R"(
      OpSource OpenCL_C 12
      OpMemoryModel Physical64 OpenCL
      OpSourceExtension "PlaceholderExtensionName"
      OpEntryPoint Kernel %0 ""
      OpExecutionMode %0 LocalSizeHint 1 1 1
      %1  = OpTypeVoid
      %2  = OpTypeBool
      %3  = OpTypeInt 8 0
      %4  = OpTypeInt 8 1
      %5  = OpTypeInt 16 0
      %6  = OpTypeInt 16 1
      %7  = OpTypeInt 32 0
      %8  = OpTypeInt 32 1
      %9  = OpTypeInt 64 0
      %10 = OpTypeInt 64 1
      %11 = OpTypeFloat 16
      %12 = OpTypeFloat 32
      %13 = OpTypeFloat 64
      %14 = OpTypeVector %3 2
  )";

  spv_binary binary = nullptr;
  spv_diagnostic diagnostic = nullptr;
  EXPECT_EQ(SPV_SUCCESS, spvTextToBinary(context, textStr, strlen(textStr),
                                         &binary, &diagnostic));
  EXPECT_NE(nullptr, binary);
  EXPECT_NE(nullptr, binary->code);
  EXPECT_NE(0u, binary->wordCount);
  if (diagnostic) {
    spvDiagnosticPrint(diagnostic);
    ASSERT_TRUE(false);
  }

  spv_text resultText = nullptr;
  EXPECT_EQ(SPV_SUCCESS,
            spvBinaryToText(context, binary->code, binary->wordCount, 0,
                            &resultText, &diagnostic));
  spvBinaryDestroy(binary);
  if (diagnostic) {
    spvDiagnosticPrint(diagnostic);
    spvDiagnosticDestroy(diagnostic);
    ASSERT_TRUE(false);
  }
  EXPECT_NE(nullptr, resultText->str);
  EXPECT_NE(0u, resultText->length);
  spvTextDestroy(resultText);
  spvContextDestroy(context);
}

}  // anonymous namespace
