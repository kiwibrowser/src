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

#include "source/spirv_constant.h"
#include "UnitSPIRV.h"

namespace {

class BinaryHeaderGet : public ::testing::Test {
 public:
  BinaryHeaderGet() { memset(code, 0, sizeof(code)); }

  virtual void SetUp() {
    code[0] = SpvMagicNumber;
    code[1] = SpvVersion;
    code[2] = SPV_GENERATOR_CODEPLAY;
    code[3] = 1;  // NOTE: Bound
    code[4] = 0;  // NOTE: Schema; reserved
    code[5] = 0;  // NOTE: Instructions

    binary.code = code;
    binary.wordCount = 6;
  }
  spv_const_binary_t get_const_binary() {
    return spv_const_binary_t{binary.code, binary.wordCount};
  }
  virtual void TearDown() {}

  uint32_t code[6];
  spv_binary_t binary;
};

TEST_F(BinaryHeaderGet, Default) {
  spv_endianness_t endian;
  spv_const_binary_t const_bin = get_const_binary();
  ASSERT_EQ(SPV_SUCCESS, spvBinaryEndianness(&const_bin, &endian));

  spv_header_t header;
  ASSERT_EQ(SPV_SUCCESS, spvBinaryHeaderGet(&const_bin, endian, &header));

  ASSERT_EQ(static_cast<uint32_t>(SpvMagicNumber), header.magic);
  ASSERT_EQ(0x00010100u, header.version);
  ASSERT_EQ(static_cast<uint32_t>(SPV_GENERATOR_CODEPLAY), header.generator);
  ASSERT_EQ(1u, header.bound);
  ASSERT_EQ(0u, header.schema);
  ASSERT_EQ(&code[5], header.instructions);
}

TEST_F(BinaryHeaderGet, InvalidCode) {
  spv_const_binary_t my_binary = {nullptr, 0};
  spv_header_t header;
  ASSERT_EQ(SPV_ERROR_INVALID_BINARY,
            spvBinaryHeaderGet(&my_binary, SPV_ENDIANNESS_LITTLE, &header));
}

TEST_F(BinaryHeaderGet, InvalidPointerHeader) {
  spv_const_binary_t const_bin = get_const_binary();
  ASSERT_EQ(SPV_ERROR_INVALID_POINTER,
            spvBinaryHeaderGet(&const_bin, SPV_ENDIANNESS_LITTLE, nullptr));
}

TEST_F(BinaryHeaderGet, TruncatedHeader) {
  for (uint8_t i = 1; i < SPV_INDEX_INSTRUCTION; i++) {
    binary.wordCount = i;
    spv_const_binary_t const_bin = get_const_binary();
    ASSERT_EQ(SPV_ERROR_INVALID_BINARY,
              spvBinaryHeaderGet(&const_bin, SPV_ENDIANNESS_LITTLE, nullptr));
  }
}

}  // anonymous namespace
