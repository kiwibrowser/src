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

#include <sstream>
#include <string>

#include "TestFixture.h"

// NOTE: The tests in this file are ONLY testing ID usage, there for the input
// SPIR-V does not follow the logical layout rules from the spec in all cases in
// order to makes the tests smaller. Validation of the whole module is handled
// in stages, ID validation is only one of these stages. All validation stages
// are stand alone.

namespace {

using ::testing::ValuesIn;
using spvtest::ScopedContext;
using std::ostringstream;
using std::string;
using std::vector;

class ValidateID : public ::testing::Test {
 public:
  virtual void TearDown() { spvBinaryDestroy(binary); }
  spv_const_binary get_const_binary() { return spv_const_binary(binary); }
  spv_binary binary;
};

const char kGLSL450MemoryModel[] = R"(
     OpCapability Shader
     OpCapability Addresses
     OpCapability Pipes
     OpCapability LiteralSampler
     OpCapability DeviceEnqueue
     OpMemoryModel Logical GLSL450
)";

const char kOpenCLMemoryModel32[] = R"(
     OpCapability Addresses
     OpCapability Linkage
     OpCapability Kernel
%1 = OpExtInstImport "OpenCL.std"
     OpMemoryModel Physical32 OpenCL
)";

const char kOpenCLMemoryModel64[] = R"(
     OpCapability Addresses
     OpCapability Linkage
     OpCapability Kernel
     OpCapability Int64
%1 = OpExtInstImport "OpenCL.std"
     OpMemoryModel Physical64 OpenCL
)";

// TODO(dekimir): this can be removed by adding a method to ValidateID akin to
// OpTypeArrayLengthTest::Val().
#define CHECK(str, expected)                                                   \
  spv_diagnostic diagnostic;                                                   \
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);               \
  std::string shader = std::string(kGLSL450MemoryModel) + str;                 \
  spv_result_t error = spvTextToBinary(context, shader.c_str(), shader.size(), \
                                       &binary, &diagnostic);                  \
  if (error) {                                                                 \
    spvDiagnosticPrint(diagnostic);                                            \
    spvDiagnosticDestroy(diagnostic);                                          \
    ASSERT_EQ(SPV_SUCCESS, error) << shader;                                   \
  }                                                                            \
  spv_result_t result = spvValidate(context, get_const_binary(), &diagnostic); \
  if (SPV_SUCCESS != result) {                                                 \
    spvDiagnosticPrint(diagnostic);                                            \
    spvDiagnosticDestroy(diagnostic);                                          \
  }                                                                            \
  ASSERT_EQ(expected, result);                                                 \
  spvContextDestroy(context);

#define CHECK_KERNEL(str, expected, bitness)                                   \
  ASSERT_TRUE(bitness == 32 || bitness == 64);                                 \
  spv_diagnostic diagnostic;                                                   \
  spv_context context = spvContextCreate(SPV_ENV_UNIVERSAL_1_0);               \
  std::string kernel = std::string(bitness == 32 ? kOpenCLMemoryModel32        \
                                                 : kOpenCLMemoryModel64) +     \
                       str;                                                    \
  spv_result_t error = spvTextToBinary(context, kernel.c_str(), kernel.size(), \
                                       &binary, &diagnostic);                  \
  if (error) {                                                                 \
    spvDiagnosticPrint(diagnostic);                                            \
    spvDiagnosticDestroy(diagnostic);                                          \
    ASSERT_EQ(SPV_SUCCESS, error);                                             \
  }                                                                            \
  spv_result_t result = spvValidate(context, get_const_binary(), &diagnostic); \
  if (SPV_SUCCESS != result) {                                                 \
    spvDiagnosticPrint(diagnostic);                                            \
    spvDiagnosticDestroy(diagnostic);                                          \
  }                                                                            \
  ASSERT_EQ(expected, result);                                                 \
  spvContextDestroy(context);

// TODO: OpUndef

TEST_F(ValidateID, OpName) {
  const char* spirv = R"(
     OpName %2 "name"
%1 = OpTypeInt 32 0
%2 = OpTypePointer UniformConstant %1
%3 = OpVariable %2 UniformConstant)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpMemberNameGood) {
  const char* spirv = R"(
     OpMemberName %2 0 "foo"
%1 = OpTypeInt 32 0
%2 = OpTypeStruct %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpMemberNameTypeBad) {
  const char* spirv = R"(
     OpMemberName %1 0 "foo"
%1 = OpTypeInt 32 0)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpMemberNameMemberBad) {
  const char* spirv = R"(
     OpMemberName %2 1 "foo"
%1 = OpTypeInt 32 0
%2 = OpTypeStruct %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpLineGood) {
  const char* spirv = R"(
%1 = OpString "/path/to/source.file"
     OpLine %1 0 0
%2 = OpTypeInt 32 0
%3 = OpTypePointer Input %2
%4 = OpVariable %3 Input)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpLineFileBad) {
  const char* spirv = R"(
     OpLine %2 0 0
%2 = OpTypeInt 32 0
%3 = OpTypePointer Input %2
%4 = OpVariable %3 Input)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpDecorateGood) {
  const char* spirv = R"(
     OpDecorate %2 GLSLShared
%1 = OpTypeInt 64 0
%2 = OpTypeStruct %1 %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpDecorateBad) {
  const char* spirv = R"(
OpDecorate %1 GLSLShared)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpMemberDecorateGood) {
  const char* spirv = R"(
     OpMemberDecorate %2 0 Uniform
%1 = OpTypeInt 32 0
%2 = OpTypeStruct %1 %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpMemberDecorateBad) {
  const char* spirv = R"(
     OpMemberDecorate %1 0 Uniform
%1 = OpTypeInt 32 0)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpMemberDecorateMemberBad) {
  const char* spirv = R"(
     OpMemberDecorate %2 3 Uniform
%1 = OpTypeInt 32 0
%2 = OpTypeStruct %1 %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpGroupDecorateGood) {
  const char* spirv = R"(
%1 = OpDecorationGroup
     OpDecorate %1 Uniform
     OpDecorate %1 GLSLShared
     OpGroupDecorate %1 %3 %4
%2 = OpTypeInt 32 0
%3 = OpConstant %2 42
%4 = OpConstant %2 23)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpGroupDecorateDecorationGroupBad) {
  const char* spirv = R"(
     OpGroupDecorate %2 %3 %4
%2 = OpTypeInt 32 0
%3 = OpConstant %2 42
%4 = OpConstant %2 23)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpGroupDecorateTargetBad) {
  const char* spirv = R"(
%1 = OpDecorationGroup
     OpDecorate %1 Uniform
     OpDecorate %1 GLSLShared
     OpGroupDecorate %1 %3
%2 = OpTypeInt 32 0)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// TODO: OpGroupMemberDecorate
// TODO: OpExtInst

TEST_F(ValidateID, OpEntryPointGood) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %3 ""
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%3 = OpFunction %1 None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd
)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpEntryPointFunctionBad) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %1 ""
%1 = OpTypeVoid)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpEntryPointParameterCountBad) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %3 ""
%1 = OpTypeVoid
%2 = OpTypeFunction %1 %1
%3 = OpFunction %1 None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpEntryPointReturnTypeBad) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %3 ""
%1 = OpTypeInt 32 0
%2 = OpTypeFunction %1
%3 = OpFunction %1 None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpExecutionModeGood) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %3 ""
     OpExecutionMode %3 LocalSize 1 1 1
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%3 = OpFunction %1 None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpExecutionModeEntryPointMissing) {
  const char* spirv = R"(
     OpExecutionMode %3 LocalSize 1 1 1
%1 = OpTypeVoid
%2 = OpTypeFunction %1
%3 = OpFunction %1 None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpExecutionModeEntryPointBad) {
  const char* spirv = R"(
     OpEntryPoint GLCompute %3 "" %a
     OpExecutionMode %a LocalSize 1 1 1
%void = OpTypeVoid
%ptr = OpTypePointer Input %void
%a = OpVariable %ptr Input
%2 = OpTypeFunction %void
%3 = OpFunction %void None %2
%4 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypeVectorFloat) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypeVector %1 4)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeVectorInt) {
  const char* spirv = R"(
%1 = OpTypeInt 32 1
%2 = OpTypeVector %1 4)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeVectorUInt) {
  const char* spirv = R"(
%1 = OpTypeInt 64 0
%2 = OpTypeVector %1 4)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeVectorBool) {
  const char* spirv = R"(
%1 = OpTypeBool
%2 = OpTypeVector %1 4)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeVectorComponentTypeBad) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypePointer UniformConstant %1
%3 = OpTypeVector %2 4)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypeMatrixGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeVector %1 2
%3 = OpTypeMatrix %2 3)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpTypeMatrixColumnTypeBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeMatrix %1 3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypeSamplerGood) {
  // In Rev31, OpTypeSampler takes no arguments.
  const char* spirv = R"(
%s = OpTypeSampler)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeArrayGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 1
%3 = OpTypeArray %1 %2)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeArrayElementTypeBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 1
%3 = OpTypeArray %2 %2)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// Signed or unsigned.
enum Signed { kSigned, kUnsigned };

// Creates an assembly snippet declaring OpTypeArray with the given length.
string MakeArrayLength(const string& len, Signed isSigned, int width) {
  ostringstream ss;
  ss << kGLSL450MemoryModel;
  ss << " %t = OpTypeInt " << width << (isSigned == kSigned ? " 1" : " 0")
     << " %l = OpConstant %t " << len << " %a = OpTypeArray %t %l";
  return ss.str();
}

// Tests OpTypeArray.  Parameter is the width (in bits) of the array-length's
// type.
class OpTypeArrayLengthTest
    : public spvtest::TextToBinaryTestBase<::testing::TestWithParam<int>> {
 protected:
  OpTypeArrayLengthTest()
      : position_(spv_position_t{0, 0, 0}),
        diagnostic_(spvDiagnosticCreate(&position_, "")) {}

  ~OpTypeArrayLengthTest() { spvDiagnosticDestroy(diagnostic_); }

  // Runs spvValidate() on v, printing any errors via spvDiagnosticPrint().
  spv_result_t Val(const SpirvVector& v) {
    spv_const_binary_t cbinary{v.data(), v.size()};
    const auto status =
        spvValidate(ScopedContext().context, &cbinary, &diagnostic_);
    if (status != SPV_SUCCESS) {
      spvDiagnosticPrint(diagnostic_);
    }
    return status;
  }

 private:
  spv_position_t position_;  // For creating diagnostic_.
  spv_diagnostic diagnostic_;
};

TEST_P(OpTypeArrayLengthTest, LengthPositive) {
  const int width = GetParam();
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("1", kSigned, width))));
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("1", kUnsigned, width))));
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("2", kSigned, width))));
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("2", kUnsigned, width))));
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("55", kSigned, width))));
  EXPECT_EQ(SPV_SUCCESS,
            Val(CompileSuccessfully(MakeArrayLength("55", kUnsigned, width))));
  const string fpad(width / 4 - 1, 'F');
  EXPECT_EQ(
      SPV_SUCCESS,
      Val(CompileSuccessfully(MakeArrayLength("0x7" + fpad, kSigned, width))));
  EXPECT_EQ(SPV_SUCCESS, Val(CompileSuccessfully(
                             MakeArrayLength("0xF" + fpad, kUnsigned, width))));
}

TEST_P(OpTypeArrayLengthTest, LengthZero) {
  const int width = GetParam();
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength("0", kSigned, width))));
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength("0", kUnsigned, width))));
}

TEST_P(OpTypeArrayLengthTest, LengthNegative) {
  const int width = GetParam();
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength("-1", kSigned, width))));
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength("-2", kSigned, width))));
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength("-123", kSigned, width))));
  const string neg_max = "0x8" + string(width / 4 - 1, '0');
  EXPECT_EQ(SPV_ERROR_INVALID_ID,
            Val(CompileSuccessfully(MakeArrayLength(neg_max, kSigned, width))));
}

INSTANTIATE_TEST_CASE_P(Widths, OpTypeArrayLengthTest,
                        ValuesIn(vector<int>{8, 16, 32, 48, 64}));

TEST_F(ValidateID, OpTypeArrayLengthNull) {
  const char* spirv = R"(
%i32 = OpTypeInt 32 1
%len = OpConstantNull %i32
%ary = OpTypeArray %i32 %len)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypeArrayLengthSpecConst) {
  const char* spirv = R"(
%i32 = OpTypeInt 32 1
%len = OpSpecConstant %i32 2
%ary = OpTypeArray %i32 %len)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeArrayLengthSpecConstOp) {
  const char* spirv = R"(
%i32 = OpTypeInt 32 1
%c1 = OpConstant %i32 1
%c2 = OpConstant %i32 2
%len = OpSpecConstantOp %i32 IAdd %c1 %c2
%ary = OpTypeArray %i32 %len)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpTypeRuntimeArrayGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeRuntimeArray %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpTypeRuntimeArrayBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 0
%3 = OpTypeRuntimeArray %2)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
// TODO: Object of this type can only be created with OpVariable using the
// Unifrom Storage Class

TEST_F(ValidateID, OpTypeStructGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeFloat 64
%3 = OpTypePointer Input %1
%4 = OpTypeStruct %1 %2 %3)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpTypeStructMemberTypeBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeFloat 64
%3 = OpConstant %2 0.0
%4 = OpTypeStruct %1 %2 %3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypePointerGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypePointer Input %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpTypePointerBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 0
%3 = OpTypePointer Input %2)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypeFunctionGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeFunction %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpTypeFunctionReturnTypeBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 0
%3 = OpTypeFunction %2)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpTypeFunctionParameterBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpConstant %2 0
%4 = OpTypeFunction %1 %2 %3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpTypePipeGood) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypeVector %1 16
%3 = OpTypePipe ReadOnly)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpConstantTrueGood) {
  const char* spirv = R"(
%1 = OpTypeBool
%2 = OpConstantTrue %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantTrueBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpConstantTrue %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantFalseGood) {
  const char* spirv = R"(
%1 = OpTypeBool
%2 = OpConstantTrue %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantFalseBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpConstantFalse %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpConstant !1 !0)";
  // The expected failure code is implementation dependent (currently
  // INVALID_BINARY because the binary parser catches these cases) and may
  // change over time, but this must always fail.
  CHECK(spirv, SPV_ERROR_INVALID_BINARY);
}

TEST_F(ValidateID, OpConstantCompositeVectorGood) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypeVector %1 4
%3 = OpConstant %1 3.14
%4 = OpConstantComposite %2 %3 %3 %3 %3)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantCompositeVectorResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypeVector %1 4
%3 = OpConstant %1 3.14
%4 = OpConstantComposite %1 %3 %3 %3 %3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeVectorConstituentBad) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpTypeVector %1 4
%4 = OpTypeInt 32 0
%3 = OpConstant %1 3.14
%5 = OpConstant %4 42
%6 = OpConstantComposite %2 %3 %5 %3 %3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeMatrixGood) {
  const char* spirv = R"(
 %1 = OpTypeFloat 32
 %2 = OpTypeVector %1 4
 %3 = OpTypeMatrix %2 4
 %4 = OpConstant %1 1.0
 %5 = OpConstant %1 0.0
 %6 = OpConstantComposite %2 %4 %5 %5 %5
 %7 = OpConstantComposite %2 %5 %4 %5 %5
 %8 = OpConstantComposite %2 %5 %5 %4 %5
 %9 = OpConstantComposite %2 %5 %5 %5 %4
%10 = OpConstantComposite %3 %6 %7 %8 %9)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantCompositeMatrixConstituentBad) {
  const char* spirv = R"(
 %1 = OpTypeFloat 32
 %2 = OpTypeVector %1 4
%11 = OpTypeVector %1 3
 %3 = OpTypeMatrix %2 4
 %4 = OpConstant %1 1.0
 %5 = OpConstant %1 0.0
 %6 = OpConstantComposite %2 %4 %5 %5 %5
 %7 = OpConstantComposite %2 %5 %4 %5 %5
 %8 = OpConstantComposite %2 %5 %5 %4 %5
 %9 = OpConstantComposite %11 %5 %5 %5
%10 = OpConstantComposite %3 %6 %7 %8 %9)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeMatrixColumnTypeBad) {
  const char* spirv = R"(
 %1 = OpTypeInt 32 0
 %2 = OpTypeFloat 32
 %3 = OpTypeVector %1 2
 %4 = OpTypeVector %3 2
 %5 = OpTypeMatrix %2 2
 %6 = OpConstant %1 42
 %7 = OpConstant %2 3.14
 %8 = OpConstantComposite %3 %6 %6
 %9 = OpConstantComposite %4 %7 %7
%10 = OpConstantComposite %5 %8 %9)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeArrayGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 4
%3 = OpTypeArray %1 %2
%4 = OpConstantComposite %3 %2 %2 %2 %2)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantCompositeArrayConstConstituentBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 4
%3 = OpTypeArray %1 %2
%4 = OpConstantComposite %3 %2 %2 %2 %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeArrayConstituentBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpConstant %1 4
%3 = OpTypeArray %1 %2
%5 = OpTypeFloat 32
%6 = OpConstant %5 3.14
%4 = OpConstantComposite %3 %2 %2 %2 %6)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpConstantCompositeStructGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeInt 64 1
%3 = OpTypeStruct %1 %1 %2
%4 = OpConstant %1 42
%5 = OpConstant %2 4300000000
%6 = OpConstantComposite %3 %4 %4 %5)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantCompositeStructMemberBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 0
%2 = OpTypeInt 64 1
%3 = OpTypeStruct %1 %1 %2
%4 = OpConstant %1 42
%5 = OpConstant %2 4300000000
%6 = OpConstantComposite %3 %4 %5 %4)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantSamplerGood) {
  const char* spirv = R"(
%float = OpTypeFloat 32
%samplerType = OpTypeSampler
%3 = OpConstantSampler %samplerType ClampToEdge 0 Nearest)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpConstantSamplerResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpConstantSampler %1 Clamp 0 Nearest)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantNullGood) {
  const char* spirv = R"(
 %1 = OpTypeBool
 %2 = OpConstantNull %1
 %3 = OpTypeInt 32 0
 %4 = OpConstantNull %3
 %5 = OpTypeFloat 32
 %6 = OpConstantNull %5
 %7 = OpTypePointer UniformConstant %3
 %8 = OpConstantNull %7
 %9 = OpTypeEvent
%10 = OpConstantNull %9
%11 = OpTypeDeviceEvent
%12 = OpConstantNull %11
%13 = OpTypeReserveId
%14 = OpConstantNull %13
%15 = OpTypeQueue
%16 = OpConstantNull %15
%17 = OpTypeVector %3 2
%18 = OpConstantNull %17
%19 = OpTypeMatrix %17 2
%20 = OpConstantNull %19
%25 = OpConstant %3 8
%21 = OpTypeArray %3 %25
%22 = OpConstantNull %21
%23 = OpTypeStruct %3 %5 %1
%24 = OpConstantNull %23
%26 = OpTypeArray %17 %25
%27 = OpConstantNull %26
%28 = OpTypeStruct %7 %26 %26 %1
%29 = OpConstantNull %28
)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpConstantNullBasicBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpConstantNull %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantNullArrayBad) {
  const char* spirv = R"(
%2 = OpTypeInt 32 0
%3 = OpTypeSampler
%4 = OpConstant %2 4
%5 = OpTypeArray %3 %4
%6 = OpConstantNull %5)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantNullStructBad) {
  const char* spirv = R"(
%2 = OpTypeSampler
%3 = OpTypeStruct %2 %2
%4 = OpConstantNull %3)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpConstantNullRuntimeArrayBad) {
  const char* spirv = R"(
%bool = OpTypeBool
%array = OpTypeRuntimeArray %bool
%null = OpConstantNull %array)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpSpecConstantTrueGood) {
  const char* spirv = R"(
%1 = OpTypeBool
%2 = OpSpecConstantTrue %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpSpecConstantTrueBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpSpecConstantTrue %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpSpecConstantFalseGood) {
  const char* spirv = R"(
%1 = OpTypeBool
%2 = OpSpecConstantFalse %1)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpSpecConstantFalseBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpSpecConstantFalse %1)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpSpecConstantGood) {
  const char* spirv = R"(
%1 = OpTypeFloat 32
%2 = OpSpecConstant %1 42)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpSpecConstantBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpSpecConstant !1 !4)";
  // The expected failure code is implementation dependent (currently
  // INVALID_BINARY because the binary parser catches these cases) and may
  // change over time, but this must always fail.
  CHECK(spirv, SPV_ERROR_INVALID_BINARY);
}

// TODO: OpSpecConstantComposite
// TODO: OpSpecConstantOp

TEST_F(ValidateID, OpVariableGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 1
%2 = OpTypePointer Input %1
%3 = OpVariable %2 Input)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpVariableInitializerGood) {
  const char* spirv = R"(
%1 = OpTypeInt 32 1
%2 = OpTypePointer Input %1
%3 = OpConstant %1 42
%4 = OpVariable %2 Input %3)";
  CHECK(spirv, SPV_SUCCESS);
}
// TODO: Positive test OpVariable with OpConstantNull of OpTypePointer
TEST_F(ValidateID, OpVariableResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 1
%2 = OpVariable %1 Input)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpVariableInitializerBad) {
  const char* spirv = R"(
%1 = OpTypeInt 32 1
%2 = OpTypePointer Input %1
%3 = OpVariable %2 Input %2)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpLoadGood) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 1
 %3 = OpTypePointer UniformConstant %2
 %4 = OpTypeFunction %1
 %5 = OpVariable %3 UniformConstant
 %6 = OpFunction %1 None %4
 %7 = OpLabel
 %8 = OpLoad %2 %5
 %9 = OpReturn
%10 = OpFunctionEnd
)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpLoadResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%5 = OpVariable %3 UniformConstant
%6 = OpFunction %1 None %4
%7 = OpLabel
%8 = OpLoad %3 %5
     OpReturn
     OpFunctionEnd
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpLoadPointerBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%9 = OpTypeFloat 32
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%6 = OpFunction %1 None %4
%7 = OpLabel
%8 = OpLoad %9 %3
     OpReturn
     OpFunctionEnd
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpStoreGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
     OpStore %6 %5
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpStorePointerBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
     OpStore %3 %5
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpStoreObjectGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
     OpStore %6 %7
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpStoreTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%9 = OpTypeFloat 32
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%5 = OpConstant %9 3.14
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
     OpStore %6 %5
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpStoreVoid) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
%9 = OpFunctionCall %1 %7
     OpStore %6 %9
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpStoreLabel) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %1
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %1 None %4
%8 = OpLabel
     OpStore %6 %8
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// TODO: enable when this bug is fixed:
// https://cvs.khronos.org/bugzilla/show_bug.cgi?id=15404
TEST_F(ValidateID, DISABLED_OpStoreFunction) {
  const char* spirv = R"(
%2 = OpTypeInt 32 1
%3 = OpTypePointer UniformConstant %2
%4 = OpTypeFunction %2
%5 = OpConstant %2 123
%6 = OpVariable %3 UniformConstant
%7 = OpFunction %2 None %4
%8 = OpLabel
     OpStore %6 %7
     OpReturnValue %5
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpCopyMemoryGood) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 0
 %3 = OpTypePointer UniformConstant %2
 %4 = OpConstant %2 42
 %5 = OpVariable %3 UniformConstant %4
 %6 = OpTypePointer Function %2
 %7 = OpTypeFunction %1
 %8 = OpFunction %1 None %7
 %9 = OpLabel
%10 = OpVariable %6 Function
      OpCopyMemory %10 %5 None
      OpReturn
      OpFunctionEnd
)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpCopyMemoryBad) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 0
 %3 = OpTypePointer UniformConstant %2
 %4 = OpConstant %2 42
 %5 = OpVariable %3 UniformConstant %4
%11 = OpTypeFloat 32
 %6 = OpTypePointer Function %11
 %7 = OpTypeFunction %1
 %8 = OpFunction %1 None %7
 %9 = OpLabel
%10 = OpVariable %6 Function
      OpCopyMemory %10 %5 None
      OpReturn
      OpFunctionEnd
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// TODO: OpCopyMemorySized
TEST_F(ValidateID, OpCopyMemorySizedGood) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 0
 %3 = OpTypePointer UniformConstant %2
 %4 = OpTypePointer Function %2
 %5 = OpConstant %2 4
 %6 = OpVariable %3 UniformConstant %5
 %7 = OpTypeFunction %1
 %8 = OpFunction %1 None %7
 %9 = OpLabel
%10 = OpVariable %4 Function
      OpCopyMemorySized %10 %6 %5 None
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpCopyMemorySizedTargetBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypePointer UniformConstant %2
%4 = OpTypePointer Function %2
%5 = OpConstant %2 4
%6 = OpVariable %3 UniformConstant %5
%7 = OpTypeFunction %1
%8 = OpFunction %1 None %7
%9 = OpLabel
     OpCopyMemorySized %9 %6 %5 None
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpCopyMemorySizedSourceBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypePointer UniformConstant %2
%4 = OpTypePointer Function %2
%5 = OpConstant %2 4
%6 = OpTypeFunction %1
%7 = OpFunction %1 None %6
%8 = OpLabel
%9 = OpVariable %4 Function
     OpCopyMemorySized %9 %6 %5 None
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpCopyMemorySizedSizeBad) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 0
 %3 = OpTypePointer UniformConstant %2
 %4 = OpTypePointer Function %2
 %5 = OpConstant %2 4
 %6 = OpVariable %3 UniformConstant %5
 %7 = OpTypeFunction %1
 %8 = OpFunction %1 None %7
 %9 = OpLabel
%10 = OpVariable %4 Function
      OpCopyMemorySized %10 %6 %6 None
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpCopyMemorySizedSizeTypeBad) {
  const char* spirv = R"(
 %1 = OpTypeVoid
 %2 = OpTypeInt 32 0
 %3 = OpTypePointer UniformConstant %2
 %4 = OpTypePointer Function %2
 %5 = OpConstant %2 4
 %6 = OpVariable %3 UniformConstant %5
 %7 = OpTypeFunction %1
%11 = OpTypeFloat 32
%12 = OpConstant %11 1.0
 %8 = OpFunction %1 None %7
 %9 = OpLabel
%10 = OpVariable %4 Function
      OpCopyMemorySized %10 %6 %12 None
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// TODO: OpAccessChain
// TODO: OpInBoundsAccessChain
// TODO: OpArrayLength
// TODO: OpImagePointer
// TODO: OpGenericPtrMemSemantics

TEST_F(ValidateID, OpFunctionGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%3 = OpTypeFunction %1 %2 %2
%4 = OpFunction %1 None %3
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpFunctionResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%5 = OpConstant %2 42
%3 = OpTypeFunction %1 %2 %2
%4 = OpFunction %2 None %3
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpFunctionFunctionTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 1
%4 = OpFunction %1 None %2
OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpFunctionParameterGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %1 %2
%4 = OpFunction %1 None %3
%5 = OpFunctionParameter %2
%6 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpFunctionParameterMultipleGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %1 %2 %2
%4 = OpFunction %1 None %3
%5 = OpFunctionParameter %2
%6 = OpFunctionParameter %2
%7 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpFunctionParameterResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %1 %2
%4 = OpFunction %1 None %3
%5 = OpFunctionParameter %1
%6 = OpLabel
     OpReturn
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpFunctionCallGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2 %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42 ;21

%6 = OpFunction %2 None %3
%7 = OpFunctionParameter %2
%8 = OpLabel
     OpReturnValue %7
     OpFunctionEnd

%10 = OpFunction %1 None %4
%11 = OpLabel
%12 = OpFunctionCall %2 %6 %5
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}
TEST_F(ValidateID, OpFunctionCallResultTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2 %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42 ;21

%6 = OpFunction %2 None %3
%7 = OpFunctionParameter %2
%8 = OpLabel
%9 = OpLoad %2 %7
     OpReturnValue %9
     OpFunctionEnd

%10 = OpFunction %1 None %4
%11 = OpLabel
%12 = OpFunctionCall %1 %6 %5
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpFunctionCallFunctionBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2 %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42 ;21

%10 = OpFunction %1 None %4
%11 = OpLabel
%12 = OpFunctionCall %2 %5 %5
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
TEST_F(ValidateID, OpFunctionCallArgumentTypeBad) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2 %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42

%13 = OpTypeFloat 32
%14 = OpConstant %13 3.14

%6 = OpFunction %2 None %3
%7 = OpFunctionParameter %2
%8 = OpLabel
%9 = OpLoad %2 %7
     OpReturnValue %9
     OpFunctionEnd

%10 = OpFunction %1 None %4
%11 = OpLabel
%12 = OpFunctionCall %2 %6 %14
      OpReturn
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
#if 0
TEST_F(ValidateID, OpFunctionCallArgumentCountBar) {
  const char *spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2 %2
%4 = OpTypeFunction %1
%5 = OpConstant %2 42 ;21

%6 = OpFunction %2 None %3
%7 = OpFunctionParameter %2
%8 = OpLabel
%9 = OpLoad %2 %7
     OpReturnValue %9
     OpFunctionEnd

%10 = OpFunction %1 None %4
%11 = OpLabel
      OpReturn
%12 = OpFunctionCall %2 %6 %5
      OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}
#endif

// TODO: OpSampledImage
// TODO: The many things that changed with how images are used.
// TODO: OpTextureSample
// TODO: OpTextureSampleDref
// TODO: OpTextureSampleLod
// TODO: OpTextureSampleProj
// TODO: OpTextureSampleGrad
// TODO: OpTextureSampleOffset
// TODO: OpTextureSampleProjLod
// TODO: OpTextureSampleProjGrad
// TODO: OpTextureSampleLodOffset
// TODO: OpTextureSampleProjOffset
// TODO: OpTextureSampleGradOffset
// TODO: OpTextureSampleProjLodOffset
// TODO: OpTextureSampleProjGradOffset
// TODO: OpTextureFetchTexelLod
// TODO: OpTextureFetchTexelOffset
// TODO: OpTextureFetchSample
// TODO: OpTextureFetchTexel
// TODO: OpTextureGather
// TODO: OpTextureGatherOffset
// TODO: OpTextureGatherOffsets
// TODO: OpTextureQuerySizeLod
// TODO: OpTextureQuerySize
// TODO: OpTextureQueryLevels
// TODO: OpTextureQuerySamples
// TODO: OpConvertUToF
// TODO: OpConvertFToS
// TODO: OpConvertSToF
// TODO: OpConvertUToF
// TODO: OpUConvert
// TODO: OpSConvert
// TODO: OpFConvert
// TODO: OpConvertPtrToU
// TODO: OpConvertUToPtr
// TODO: OpPtrCastToGeneric
// TODO: OpGenericCastToPtr
// TODO: OpBitcast
// TODO: OpGenericCastToPtrExplicit
// TODO: OpSatConvertSToU
// TODO: OpSatConvertUToS
// TODO: OpVectorExtractDynamic
// TODO: OpVectorInsertDynamic
// TODO: OpVectorShuffle
// TODO: OpCompositeConstruct
// TODO: OpCompositeExtract
// TODO: OpCompositeInsert
// TODO: OpCopyObject
// TODO: OpTranspose
// TODO: OpSNegate
// TODO: OpFNegate
// TODO: OpNot
// TODO: OpIAdd
// TODO: OpFAdd
// TODO: OpISub
// TODO: OpFSub
// TODO: OpIMul
// TODO: OpFMul
// TODO: OpUDiv
// TODO: OpSDiv
// TODO: OpFDiv
// TODO: OpUMod
// TODO: OpSRem
// TODO: OpSMod
// TODO: OpFRem
// TODO: OpFMod
// TODO: OpVectorTimesScalar
// TODO: OpMatrixTimesScalar
// TODO: OpVectorTimesMatrix
// TODO: OpMatrixTimesVector
// TODO: OpMatrixTimesMatrix
// TODO: OpOuterProduct
// TODO: OpDot
// TODO: OpShiftRightLogical
// TODO: OpShiftRightArithmetic
// TODO: OpShiftLeftLogical
// TODO: OpBitwiseOr
// TODO: OpBitwiseXor
// TODO: OpBitwiseAnd
// TODO: OpAny
// TODO: OpAll
// TODO: OpIsNan
// TODO: OpIsInf
// TODO: OpIsFinite
// TODO: OpIsNormal
// TODO: OpSignBitSet
// TODO: OpLessOrGreater
// TODO: OpOrdered
// TODO: OpUnordered
// TODO: OpLogicalOr
// TODO: OpLogicalXor
// TODO: OpLogicalAnd
// TODO: OpSelect
// TODO: OpIEqual
// TODO: OpFOrdEqual
// TODO: OpFUnordEqual
// TODO: OpINotEqual
// TODO: OpFOrdNotEqual
// TODO: OpFUnordNotEqual
// TODO: OpULessThan
// TODO: OpSLessThan
// TODO: OpFOrdLessThan
// TODO: OpFUnordLessThan
// TODO: OpUGreaterThan
// TODO: OpSGreaterThan
// TODO: OpFOrdGreaterThan
// TODO: OpFUnordGreaterThan
// TODO: OpULessThanEqual
// TODO: OpSLessThanEqual
// TODO: OpFOrdLessThanEqual
// TODO: OpFUnordLessThanEqual
// TODO: OpUGreaterThanEqual
// TODO: OpSGreaterThanEqual
// TODO: OpFOrdGreaterThanEqual
// TODO: OpFUnordGreaterThanEqual
// TODO: OpDPdx
// TODO: OpDPdy
// TODO: OpFWidth
// TODO: OpDPdxFine
// TODO: OpDPdyFine
// TODO: OpFwidthFine
// TODO: OpDPdxCoarse
// TODO: OpDPdyCoarse
// TODO: OpFwidthCoarse
// TODO: OpPhi
// TODO: OpLoopMerge
// TODO: OpSelectionMerge
// TODO: OpBranch
// TODO: OpBranchConditional
// TODO: OpSwitch

TEST_F(ValidateID, OpReturnValueConstantGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2
%4 = OpConstant %2 42
%5 = OpFunction %2 None %3
%6 = OpLabel
     OpReturnValue %4
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpReturnValueVariableGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0 ;10
%3 = OpTypeFunction %2
%8 = OpTypePointer Function %2 ;18
%4 = OpConstant %2 42 ;22
%5 = OpFunction %2 None %3 ;27
%6 = OpLabel ;29
%7 = OpVariable %8 Function %4 ;34
%9 = OpLoad %2 %7
     OpReturnValue %9 ;36
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpReturnValueExpressionGood) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2
%4 = OpConstant %2 42
%5 = OpFunction %2 None %3
%6 = OpLabel
%7 = OpIAdd %2 %4 %4
     OpReturnValue %7
     OpFunctionEnd)";
  CHECK(spirv, SPV_SUCCESS);
}

TEST_F(ValidateID, OpReturnValueIsType) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2
%5 = OpFunction %2 None %3
%6 = OpLabel
     OpReturnValue %1
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpReturnValueIsLabel) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2
%5 = OpFunction %2 None %3
%6 = OpLabel
     OpReturnValue %6
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpReturnValueIsVoid) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %1
%5 = OpFunction %1 None %3
%6 = OpLabel
%7 = OpFunctionCall %1 %5
     OpReturnValue %7
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, OpReturnValueIsVariable) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypePointer Private %2
%4 = OpTypeFunction %3
%5 = OpFunction %3 None %4
%6 = OpLabel
%7 = OpVariable %3 Function
     OpReturnValue %7
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

// TODO: enable when this bug is fixed:
// https://cvs.khronos.org/bugzilla/show_bug.cgi?id=15404
TEST_F(ValidateID, DISABLED_OpReturnValueIsFunction) {
  const char* spirv = R"(
%1 = OpTypeVoid
%2 = OpTypeInt 32 0
%3 = OpTypeFunction %2
%5 = OpFunction %2 None %3
%6 = OpLabel
     OpReturnValue %5
     OpFunctionEnd)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, UndefinedTypeId) {
  const char* spirv = R"(
%s = OpTypeStruct %i32
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, UndefinedIdScope) {
  const char* spirv = R"(
%u32    = OpTypeInt 32 0
%memsem = OpConstant %u32 0
%void   = OpTypeVoid
%void_f = OpTypeFunction %void
%f      = OpFunction %void None %void_f
%l      = OpLabel
          OpMemoryBarrier %undef %memsem
          OpReturn
          OpFunctionEnd
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, UndefinedIdMemSem) {
  const char* spirv = R"(
%u32    = OpTypeInt 32 0
%scope  = OpConstant %u32 0
%void   = OpTypeVoid
%void_f = OpTypeFunction %void
%f      = OpFunction %void None %void_f
%l      = OpLabel
          OpMemoryBarrier %scope %undef
          OpReturn
          OpFunctionEnd
)";
  CHECK(spirv, SPV_ERROR_INVALID_ID);
}

TEST_F(ValidateID, KernelOpEntryPointAndOpInBoundsPtrAccessChainGood) {
  const char* spirv = R"(
      OpEntryPoint Kernel %2 "simple_kernel"
      OpSource OpenCL_C 200000
      OpDecorate %3 BuiltIn GlobalInvocationId
      OpDecorate %3 Constant
      OpDecorate %4 FuncParamAttr NoCapture
      OpDecorate %3 LinkageAttributes "__spirv_GlobalInvocationId" Import
 %5 = OpTypeInt 32 0
 %6 = OpTypeVector %5 3
 %7 = OpTypePointer UniformConstant %6
 %3 = OpVariable %7 UniformConstant
 %8 = OpTypeVoid
 %9 = OpTypeStruct %5
%10 = OpTypePointer CrossWorkgroup %9
%11 = OpTypeFunction %8 %10
%12 = OpConstant %5 0
%13 = OpTypePointer CrossWorkgroup %5
%14 = OpConstant %5 42
 %2 = OpFunction %8 None %11
 %4 = OpFunctionParameter %10
%15 = OpLabel
%16 = OpLoad %6 %3 Aligned 0
%17 = OpCompositeExtract %5 %16 0
%18 = OpInBoundsPtrAccessChain %13 %4 %17 %12
      OpStore %18 %14 Aligned 4
      OpReturn
      OpFunctionEnd)";
  CHECK_KERNEL(spirv, SPV_SUCCESS, 32);
}

TEST_F(ValidateID, OpPtrAccessChainGood) {
  const char* spirv = R"(
      OpEntryPoint Kernel %2 "another_kernel"
      OpSource OpenCL_C 200000
      OpDecorate %3 BuiltIn GlobalInvocationId
      OpDecorate %3 Constant
      OpDecorate %4 FuncParamAttr NoCapture
      OpDecorate %3 LinkageAttributes "__spirv_GlobalInvocationId" Import
 %5 = OpTypeInt 64 0
 %6 = OpTypeVector %5 3
 %7 = OpTypePointer UniformConstant %6
 %3 = OpVariable %7 UniformConstant
 %8 = OpTypeVoid
 %9 = OpTypeInt 32 0
%10 = OpTypeStruct %9
%11 = OpTypePointer CrossWorkgroup %10
%12 = OpTypeFunction %8 %11
%13 = OpConstant %5 4294967295
%14 = OpConstant %9 0
%15 = OpTypePointer CrossWorkgroup %9
%16 = OpConstant %9 42
 %2 = OpFunction %8 None %12
 %4 = OpFunctionParameter %11
%17 = OpLabel
%18 = OpLoad %6 %3 Aligned 0
%19 = OpCompositeExtract %5 %18 0
%20 = OpBitwiseAnd %5 %19 %13
%21 = OpPtrAccessChain %15 %4 %20 %14
      OpStore %21 %16 Aligned 4
      OpReturn
      OpFunctionEnd)";
  CHECK_KERNEL(spirv, SPV_SUCCESS, 64);
}

// TODO: OpLifetimeStart
// TODO: OpLifetimeStop
// TODO: OpAtomicInit
// TODO: OpAtomicLoad
// TODO: OpAtomicStore
// TODO: OpAtomicExchange
// TODO: OpAtomicCompareExchange
// TODO: OpAtomicCompareExchangeWeak
// TODO: OpAtomicIIncrement
// TODO: OpAtomicIDecrement
// TODO: OpAtomicIAdd
// TODO: OpAtomicISub
// TODO: OpAtomicUMin
// TODO: OpAtomicUMax
// TODO: OpAtomicAnd
// TODO: OpAtomicOr
// TODO: OpAtomicXor
// TODO: OpAtomicIMin
// TODO: OpAtomicIMax
// TODO: OpEmitStreamVertex
// TODO: OpEndStreamPrimitive
// TODO: OpAsyncGroupCopy
// TODO: OpWaitGroupEvents
// TODO: OpGroupAll
// TODO: OpGroupAny
// TODO: OpGroupBroadcast
// TODO: OpGroupIAdd
// TODO: OpGroupFAdd
// TODO: OpGroupFMin
// TODO: OpGroupUMin
// TODO: OpGroupSMin
// TODO: OpGroupFMax
// TODO: OpGroupUMax
// TODO: OpGroupSMax
// TODO: OpEnqueueMarker
// TODO: OpEnqueueKernel
// TODO: OpGetKernelNDrangeSubGroupCount
// TODO: OpGetKernelNDrangeMaxSubGroupSize
// TODO: OpGetKernelWorkGroupSize
// TODO: OpGetKernelPreferredWorkGroupSizeMultiple
// TODO: OpRetainEvent
// TODO: OpReleaseEvent
// TODO: OpCreateUserEvent
// TODO: OpIsValidEvent
// TODO: OpSetUserEventStatus
// TODO: OpCaptureEventProfilingInfo
// TODO: OpGetDefaultQueue
// TODO: OpBuildNDRange
// TODO: OpReadPipe
// TODO: OpWritePipe
// TODO: OpReservedReadPipe
// TODO: OpReservedWritePipe
// TODO: OpReserveReadPipePackets
// TODO: OpReserveWritePipePackets
// TODO: OpCommitReadPipe
// TODO: OpCommitWritePipe
// TODO: OpIsValidReserveId
// TODO: OpGetNumPipePackets
// TODO: OpGetMaxPipePackets
// TODO: OpGroupReserveReadPipePackets
// TODO: OpGroupReserveWritePipePackets
// TODO: OpGroupCommitReadPipe
// TODO: OpGroupCommitWritePipe

}  // anonymous namespace
