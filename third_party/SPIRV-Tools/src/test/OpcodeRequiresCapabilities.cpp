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

class Requires : public ::testing::TestWithParam<SpvCapability> {
 public:
  Requires()
      : entry({nullptr,
               (SpvOp)0,
               SPV_CAPABILITY_AS_MASK(GetParam()),
               0,
               {},
               false,
               false}) {}

  virtual void SetUp() {}

  virtual void TearDown() {}

  spv_opcode_desc_t entry;
};

TEST_P(Requires, Capabilities) {
  ASSERT_NE(0, spvOpcodeRequiresCapabilities(&entry));
}

INSTANTIATE_TEST_CASE_P(
    Op, Requires,
    ::testing::Values(SpvCapabilityMatrix, SpvCapabilityShader,
                      SpvCapabilityGeometry, SpvCapabilityTessellation,
                      SpvCapabilityAddresses, SpvCapabilityLinkage,
                      SpvCapabilityKernel,
                      // ClipDistance has enum value 32.
                      // So it tests that we are sensitive
                      // to more than just the least
                      // significant 32 bits of the
                      // capability mask.
                      SpvCapabilityClipDistance,
                      // Transformfeedback has value 53,
                      // and is the last capability.
                      SpvCapabilityTransformFeedback), );

TEST(OpcodeRequiresCapability, None) {
  spv_opcode_desc_t entry = {nullptr, (SpvOp)0, 0, 0, {}, false, false};
  ASSERT_EQ(0, spvOpcodeRequiresCapabilities(&entry));
}

/// Test SPV_CAPBILITY_AS_MASK

TEST(CapabilityAsMaskMacro, Sample) {
  EXPECT_EQ(uint64_t(1), SPV_CAPABILITY_AS_MASK(SpvCapabilityMatrix));
  EXPECT_EQ(uint64_t(0x8000), SPV_CAPABILITY_AS_MASK(SpvCapabilityImageMipmap));
  EXPECT_EQ(uint64_t(0x100000000ULL),
            SPV_CAPABILITY_AS_MASK(SpvCapabilityClipDistance));
  EXPECT_EQ(uint64_t(1) << 53,
            SPV_CAPABILITY_AS_MASK(SpvCapabilityTransformFeedback));
}

/// Capabilities required by an Opcode.
struct ExpectedOpCodeCapabilities {
  SpvOp opcode;
  uint64_t capabilities;  //< Bitfield of SpvCapability.
};

using OpcodeTableCapabilitiesTest =
    ::testing::TestWithParam<ExpectedOpCodeCapabilities>;

TEST_P(OpcodeTableCapabilitiesTest, TableEntryMatchesExpectedCapabilities) {
  spv_opcode_table opcodeTable;
  ASSERT_EQ(SPV_SUCCESS,
            spvOpcodeTableGet(&opcodeTable, SPV_ENV_UNIVERSAL_1_1));
  spv_opcode_desc entry;
  ASSERT_EQ(SPV_SUCCESS,
            spvOpcodeTableValueLookup(opcodeTable, GetParam().opcode, &entry));
  EXPECT_EQ(GetParam().capabilities, entry->capabilities);
}

/// Translates a SpvCapability into a bitfield.
inline uint64_t mask(SpvCapability c) { return SPV_CAPABILITY_AS_MASK(c); }

/// Combines two SpvCapabilities into a bitfield.
inline uint64_t mask(SpvCapability c1, SpvCapability c2) {
  return SPV_CAPABILITY_AS_MASK(c1) | SPV_CAPABILITY_AS_MASK(c2);
}

INSTANTIATE_TEST_CASE_P(
    TableRowTest, OpcodeTableCapabilitiesTest,
    // Spot-check a few opcodes.
    ::testing::Values(
        ExpectedOpCodeCapabilities{
            SpvOpImageQuerySize,
            mask(SpvCapabilityKernel, SpvCapabilityImageQuery)},
        ExpectedOpCodeCapabilities{
            SpvOpImageQuerySizeLod,
            mask(SpvCapabilityKernel, SpvCapabilityImageQuery)},
        ExpectedOpCodeCapabilities{
            SpvOpImageQueryLevels,
            mask(SpvCapabilityKernel, SpvCapabilityImageQuery)},
        ExpectedOpCodeCapabilities{
            SpvOpImageQuerySamples,
            mask(SpvCapabilityKernel, SpvCapabilityImageQuery)},
        ExpectedOpCodeCapabilities{SpvOpImageSparseSampleImplicitLod,
                                   mask(SpvCapabilitySparseResidency)},
        ExpectedOpCodeCapabilities{SpvOpCopyMemorySized,
                                   mask(SpvCapabilityAddresses)},
        ExpectedOpCodeCapabilities{SpvOpArrayLength, mask(SpvCapabilityShader)},
        ExpectedOpCodeCapabilities{SpvOpFunction, 0},
        ExpectedOpCodeCapabilities{SpvOpConvertFToS, 0},
        ExpectedOpCodeCapabilities{SpvOpEmitStreamVertex,
                                   mask(SpvCapabilityGeometryStreams)},
        ExpectedOpCodeCapabilities{SpvOpTypeNamedBarrier,
                                   mask(SpvCapabilityNamedBarrier)},
        ExpectedOpCodeCapabilities{SpvOpGetKernelMaxNumSubgroups,
                                   mask(SpvCapabilitySubgroupDispatch)}), );

}  // anonymous namespace
