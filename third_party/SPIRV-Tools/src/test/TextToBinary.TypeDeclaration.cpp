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

// Assembler tests for instructions in the "Type-Declaration" section of the
// SPIR-V spec.

#include "UnitSPIRV.h"

#include "TestFixture.h"
#include "gmock/gmock.h"

namespace {

using spvtest::EnumCase;
using spvtest::MakeInstruction;
using ::testing::Eq;

// Test Dim enums via OpTypeImage

using DimTest =
    spvtest::TextToBinaryTestBase<::testing::TestWithParam<EnumCase<SpvDim>>>;

TEST_P(DimTest, AnyDim) {
  const std::string input =
      "%1 = OpTypeImage %2 " + GetParam().name() + " 2 3 0 4 Rgba8\n";
  EXPECT_THAT(
      CompiledInstructions(input),
      Eq(MakeInstruction(SpvOpTypeImage, {1, 2, GetParam().value(), 2, 3, 0, 4,
                                          SpvImageFormatRgba8})));

  // Check the disassembler as well.
  EXPECT_THAT(EncodeAndDecodeSuccessfully(input), Eq(input));
}

// clang-format off
#define CASE(NAME) {SpvDim##NAME, #NAME}
INSTANTIATE_TEST_CASE_P(
    TextToBinaryDim, DimTest,
    ::testing::ValuesIn(std::vector<EnumCase<SpvDim>>{
        CASE(1D),
        CASE(2D),
        CASE(3D),
        CASE(Cube),
        CASE(Rect),
        CASE(Buffer),
        CASE(SubpassData),
    }),);
#undef CASE
// clang-format on

TEST_F(DimTest, WrongDim) {
  EXPECT_THAT(CompileFailure("%i = OpTypeImage %t xxyyzz 1 2 3 4 R8"),
              Eq("Invalid dimensionality 'xxyyzz'."));
}

// Test ImageFormat enums via OpTypeImage

using ImageFormatTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<SpvImageFormat>>>;

TEST_P(ImageFormatTest, AnyImageFormatAndNoAccessQualifier) {
  const std::string input =
      "%1 = OpTypeImage %2 1D 2 3 0 4 " + GetParam().name() + "\n";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpTypeImage, {1, 2, SpvDim1D, 2, 3, 0, 4,
                                                  GetParam().value()})));
  // Check the disassembler as well.
  EXPECT_THAT(EncodeAndDecodeSuccessfully(input), Eq(input));
}

// clang-format off
#define CASE(NAME) {SpvImageFormat##NAME, #NAME}
INSTANTIATE_TEST_CASE_P(
    TextToBinaryImageFormat, ImageFormatTest,
    ::testing::ValuesIn(std::vector<EnumCase<SpvImageFormat>>{
        CASE(Unknown),
        CASE(Rgba32f),
        CASE(Rgba16f),
        CASE(R32f),
        CASE(Rgba8),
        CASE(Rgba8Snorm),
        CASE(Rg32f),
        CASE(Rg16f),
        CASE(R11fG11fB10f),
        CASE(R16f),
        CASE(Rgba16),
        CASE(Rgb10A2),
        CASE(Rg16),
        CASE(Rg8),
        CASE(R16),
        CASE(R8),
        CASE(Rgba16Snorm),
        CASE(Rg16Snorm),
        CASE(Rg8Snorm),
        CASE(R16Snorm),
        CASE(R8Snorm),
        CASE(Rgba32i),
        CASE(Rgba16i),
        CASE(Rgba8i),
        CASE(R32i),
        CASE(Rg32i),
        CASE(Rg16i),
        CASE(Rg8i),
        CASE(R16i),
        CASE(R8i),
        CASE(Rgba32ui),
        CASE(Rgba16ui),
        CASE(Rgba8ui),
        CASE(R32ui),
        CASE(Rgb10a2ui),
        CASE(Rg32ui),
        CASE(Rg16ui),
        CASE(Rg8ui),
        CASE(R16ui),
        CASE(R8ui),
    }),);
#undef CASE
// clang-format on

TEST_F(ImageFormatTest, WrongFormat) {
  EXPECT_THAT(CompileFailure("%r = OpTypeImage %t 1D  2 3 0 4 xxyyzz"),
              Eq("Invalid image format 'xxyyzz'."));
}

// Test AccessQualifier enums via OpTypeImage.
using ImageAccessQualifierTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<SpvAccessQualifier>>>;

TEST_P(ImageAccessQualifierTest, AnyAccessQualifier) {
  const std::string input =
      "%1 = OpTypeImage %2 1D 2 3 0 4 Rgba8 " + GetParam().name() + "\n";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpTypeImage,
                                 {1, 2, SpvDim1D, 2, 3, 0, 4,
                                  SpvImageFormatRgba8, GetParam().value()})));
  // Check the disassembler as well.
  EXPECT_THAT(EncodeAndDecodeSuccessfully(input), Eq(input));
}

// clang-format off
#define CASE(NAME) {SpvAccessQualifier##NAME, #NAME}
INSTANTIATE_TEST_CASE_P(
    AccessQualifier, ImageAccessQualifierTest,
    ::testing::ValuesIn(std::vector<EnumCase<SpvAccessQualifier>>{
      CASE(ReadOnly),
      CASE(WriteOnly),
      CASE(ReadWrite),
    }),);
// clang-format on
#undef CASE

// Test AccessQualifier enums via OpTypePipe.

using OpTypePipeTest = spvtest::TextToBinaryTestBase<
    ::testing::TestWithParam<EnumCase<SpvAccessQualifier>>>;

TEST_P(OpTypePipeTest, AnyAccessQualifier) {
  const std::string input = "%1 = OpTypePipe " + GetParam().name() + "\n";
  EXPECT_THAT(CompiledInstructions(input),
              Eq(MakeInstruction(SpvOpTypePipe, {1, GetParam().value()})));
  // Check the disassembler as well.
  EXPECT_THAT(EncodeAndDecodeSuccessfully(input), Eq(input));
}

// clang-format off
#define CASE(NAME) {SpvAccessQualifier##NAME, #NAME}
INSTANTIATE_TEST_CASE_P(
    TextToBinaryTypePipe, OpTypePipeTest,
    ::testing::ValuesIn(std::vector<EnumCase<SpvAccessQualifier>>{
                            CASE(ReadOnly),
                            CASE(WriteOnly),
                            CASE(ReadWrite),
    }),);
#undef CASE
// clang-format on

TEST_F(OpTypePipeTest, WrongAccessQualifier) {
  EXPECT_THAT(CompileFailure("%1 = OpTypePipe xxyyzz"),
              Eq("Invalid access qualifier 'xxyyzz'."));
}

using OpTypeForwardPointerTest = spvtest::TextToBinaryTest;

#define CASE(storage_class)                                               \
  do {                                                                    \
    EXPECT_THAT(                                                          \
        CompiledInstructions("OpTypeForwardPointer %pt " #storage_class), \
        Eq(MakeInstruction(SpvOpTypeForwardPointer,                       \
                           {1, SpvStorageClass##storage_class})));        \
  } while (0)

TEST_F(OpTypeForwardPointerTest, ValidStorageClass) {
  CASE(UniformConstant);
  CASE(Input);
  CASE(Uniform);
  CASE(Output);
  CASE(Workgroup);
  CASE(CrossWorkgroup);
  CASE(Private);
  CASE(Function);
  CASE(Generic);
  CASE(PushConstant);
  CASE(AtomicCounter);
  CASE(Image);
}

#undef CASE

TEST_F(OpTypeForwardPointerTest, MissingType) {
  EXPECT_THAT(CompileFailure("OpTypeForwardPointer"),
              Eq("Expected operand, found end of stream."));
}

TEST_F(OpTypeForwardPointerTest, MissingClass) {
  EXPECT_THAT(CompileFailure("OpTypeForwardPointer %pt"),
              Eq("Expected operand, found end of stream."));
}

TEST_F(OpTypeForwardPointerTest, WrongClass) {
  EXPECT_THAT(CompileFailure("OpTypeForwardPointer %pt xxyyzz"),
              Eq("Invalid storage class 'xxyyzz'."));
}

// TODO(dneto): OpTypeVoid
// TODO(dneto): OpTypeBool
// TODO(dneto): OpTypeInt
// TODO(dneto): OpTypeFloat
// TODO(dneto): OpTypeVector
// TODO(dneto): OpTypeMatrix
// TODO(dneto): OpTypeImage
// TODO(dneto): OpTypeSampler
// TODO(dneto): OpTypeSampledImage
// TODO(dneto): OpTypeArray
// TODO(dneto): OpTypeRuntimeArray
// TODO(dneto): OpTypeStruct
// TODO(dneto): OpTypeOpaque
// TODO(dneto): OpTypePointer
// TODO(dneto): OpTypeFunction
// TODO(dneto): OpTypeEvent
// TODO(dneto): OpTypeDeviceEvent
// TODO(dneto): OpTypeReserveId
// TODO(dneto): OpTypeQueue

}  // anonymous namespace
