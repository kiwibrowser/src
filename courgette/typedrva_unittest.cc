// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "courgette/base_test_unittest.h"
#include "courgette/disassembler_elf_32_arm.h"
#include "courgette/disassembler_elf_32_x86.h"
#include "courgette/image_utils.h"

class TypedRVATest : public BaseTest {
 public:
  void TestRelativeTargetX86(courgette::RVA word, courgette::RVA expected)
    const;

  void TestRelativeTargetARM(courgette::ARM_RVA arm_rva,
                             courgette::RVA rva,
                             uint32_t op,
                             courgette::RVA expected) const;

  void TestARMOPEncode(courgette::ARM_RVA arm_rva,
                       courgette::RVA rva,
                       uint32_t op,
                       courgette::RVA expected) const;
};

void TypedRVATest::TestRelativeTargetX86(courgette::RVA word,
                                         courgette::RVA expected) const {
  courgette::DisassemblerElf32X86::TypedRVAX86* typed_rva
    = new courgette::DisassemblerElf32X86::TypedRVAX86(0);
  const uint8_t* op_pointer = reinterpret_cast<const uint8_t*>(&word);

  EXPECT_TRUE(typed_rva->ComputeRelativeTarget(op_pointer));
  EXPECT_EQ(typed_rva->relative_target(), expected);

  delete typed_rva;
}

uint32_t Read32LittleEndian(const void* address) {
  return *reinterpret_cast<const uint32_t*>(address);
}

void TypedRVATest::TestRelativeTargetARM(courgette::ARM_RVA arm_rva,
                                         courgette::RVA rva,
                                         uint32_t op,
                                         courgette::RVA expected) const {
  courgette::DisassemblerElf32ARM::TypedRVAARM* typed_rva
    = new courgette::DisassemblerElf32ARM::TypedRVAARM(arm_rva, rva);
  uint8_t* op_pointer = reinterpret_cast<uint8_t*>(&op);

  EXPECT_TRUE(typed_rva->ComputeRelativeTarget(op_pointer));
  EXPECT_EQ(rva + typed_rva->relative_target(), expected);

  delete typed_rva;
}

void TypedRVATest::TestARMOPEncode(courgette::ARM_RVA arm_rva,
                                   courgette::RVA rva,
                                   uint32_t op,
                                   courgette::RVA expected) const {
  uint16_t c_op;
  uint32_t addr;
  EXPECT_TRUE(courgette::DisassemblerElf32ARM::Compress(arm_rva, op, rva,
                                                        &c_op, &addr));
  EXPECT_EQ(rva + addr, expected);

  uint32_t new_op;
  EXPECT_TRUE(courgette::DisassemblerElf32ARM::Decompress(arm_rva, c_op, addr,
                                                          &new_op));
  EXPECT_EQ(new_op, op);
}

TEST_F(TypedRVATest, TestX86) {
  TestRelativeTargetX86(0x0, 0x4);
}

// ARM opcodes taken from and tested against the output of
// "arm-linux-gnueabi-objdump -d daisy_3701.98.0/bin/ls"

TEST_F(TypedRVATest, TestARM_OFF8_PREFETCH) {
  TestRelativeTargetARM(courgette::ARM_OFF8, 0x0, 0x0, 0x4);
}

TEST_F(TypedRVATest, TestARM_OFF8_FORWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF8, 0x2bcc, 0xd00e, 0x2bec);
  TestRelativeTargetARM(courgette::ARM_OFF8, 0x3752, 0xd910, 0x3776);
}

TEST_F(TypedRVATest, TestARM_OFF8_BACKWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF8, 0x3774, 0xd1f6, 0x3764);
}

TEST_F(TypedRVATest, TestARM_OFF11_PREFETCH) {
  TestRelativeTargetARM(courgette::ARM_OFF11, 0x0, 0x0, 0x4);
}

TEST_F(TypedRVATest, TestARM_OFF11_FORWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF11, 0x2bea, 0xe005, 0x2bf8);
}

TEST_F(TypedRVATest, TestARM_OFF11_BACKWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF11, 0x2f80, 0xe6cd, 0x2d1e);
  TestRelativeTargetARM(courgette::ARM_OFF11, 0x3610, 0xe56a, 0x30e8);
}

TEST_F(TypedRVATest, TestARM_OFF24_PREFETCH) {
  TestRelativeTargetARM(courgette::ARM_OFF24, 0x0, 0x0, 0x8);
}

TEST_F(TypedRVATest, TestARM_OFF24_FORWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF24, 0x2384, 0x4af3613a, 0xffcda874);
  TestRelativeTargetARM(courgette::ARM_OFF24, 0x23bc, 0x6af961b9, 0xffe5aaa8);
  TestRelativeTargetARM(courgette::ARM_OFF24, 0x23d4, 0x2b006823, 0x1c468);
}

TEST_F(TypedRVATest, TestARM_OFF24_BACKWARDS) {
  // TODO(paulgazz): find a real-world example of an non-thumb ARM
  // branch op that jumps backwards.
}

TEST_F(TypedRVATest, TestARM_OFF25_FORWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF25, 0x2bf4, 0xfe06f008, 0xb804);
  TestRelativeTargetARM(courgette::ARM_OFF25, 0x2c58, 0xfeacf005, 0x89b4);
}

TEST_F(TypedRVATest, TestARM_OFF25_BACKWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF25, 0x2bd2, 0xeb9ef7ff, 0x2310);
  TestRelativeTargetARM(courgette::ARM_OFF25, 0x2bd8, 0xeb8ef7ff, 0x22f8);
  TestRelativeTargetARM(courgette::ARM_OFF25, 0x2c3e, 0xea2ef7ff, 0x209c);
}

TEST_F(TypedRVATest, TestARM_OFF21_FORWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF21, 0x2bc6, 0x84c7f000, 0x3558);
  TestRelativeTargetARM(courgette::ARM_OFF21, 0x2bde, 0x871df000, 0x3a1c);
  TestRelativeTargetARM(courgette::ARM_OFF21, 0x2c5e, 0x86c1f2c0, 0x39e4);
}

TEST_F(TypedRVATest, TestARM_OFF21_BACKWARDS) {
  TestRelativeTargetARM(courgette::ARM_OFF21, 0x67e4, 0xaee9f43f, 0x65ba);
  TestRelativeTargetARM(courgette::ARM_OFF21, 0x67ee, 0xaee4f47f, 0x65ba);
}

TEST_F(TypedRVATest, TestARMOPEncode) {
  TestARMOPEncode(courgette::ARM_OFF8, 0x2bcc, 0xd00e, 0x2bec);
  TestARMOPEncode(courgette::ARM_OFF8, 0x3752, 0xd910, 0x3776);
  TestARMOPEncode(courgette::ARM_OFF8, 0x3774, 0xd1f6, 0x3764);
  TestARMOPEncode(courgette::ARM_OFF11, 0x0, 0x0, 0x4);
  TestARMOPEncode(courgette::ARM_OFF11, 0x2bea, 0xe005, 0x2bf8);
  TestARMOPEncode(courgette::ARM_OFF11, 0x2f80, 0xe6cd, 0x2d1e);
  TestARMOPEncode(courgette::ARM_OFF11, 0x3610, 0xe56a, 0x30e8);
  TestARMOPEncode(courgette::ARM_OFF24, 0x0, 0x0, 0x8);
  TestARMOPEncode(courgette::ARM_OFF24, 0x2384, 0x4af3613a, 0xffcda874);
  TestARMOPEncode(courgette::ARM_OFF24, 0x23bc, 0x6af961b9, 0xffe5aaa8);
  TestARMOPEncode(courgette::ARM_OFF24, 0x23d4, 0x2b006823, 0x1c468);
  TestARMOPEncode(courgette::ARM_OFF25, 0x2bf4, 0xf008fe06, 0xb804);
  TestARMOPEncode(courgette::ARM_OFF25, 0x2c58, 0xf005feac, 0x89b4);
  TestARMOPEncode(courgette::ARM_OFF25, 0x2bd2, 0xf7ffeb9e, 0x2310);
  TestARMOPEncode(courgette::ARM_OFF25, 0x2bd8, 0xf7ffeb8e, 0x22f8);
  TestARMOPEncode(courgette::ARM_OFF25, 0x2c3e, 0xf7ffea2e, 0x209c);
  TestARMOPEncode(courgette::ARM_OFF21, 0x2bc6, 0xf00084c7, 0x3558);
  TestARMOPEncode(courgette::ARM_OFF21, 0x2bde, 0xf000871d, 0x3a1c);
  TestARMOPEncode(courgette::ARM_OFF21, 0x2c5e, 0xf2c086c1, 0x39e4);
  TestARMOPEncode(courgette::ARM_OFF21, 0x67e4, 0xf43faee9, 0x65ba);
  TestARMOPEncode(courgette::ARM_OFF21, 0x67ee, 0xf47faee4, 0x65ba);
}
