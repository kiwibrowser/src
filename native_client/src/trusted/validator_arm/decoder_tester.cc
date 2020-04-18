/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests the decoder.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

#include "native_client/src/trusted/validator_arm/decoder_tester.h"

#include <string>
#include "gtest/gtest.h"

using nacl_arm_dec::kArm32InstSize;
using nacl_arm_dec::Instruction;
using nacl_arm_dec::MAY_BE_SAFE;

namespace nacl_arm_test {

DecoderTester::DecoderTester()
{}

bool DecoderTester::PassesParsePreconditions(
    nacl_arm_dec::Instruction inst,
    const NamedClassDecoder& decoder) {
  UNREFERENCED_PARAMETER(inst);
  UNREFERENCED_PARAMETER(decoder);
  return true;
}

bool DecoderTester::ApplySanityChecks(Instruction inst,
                                      const NamedClassDecoder& decoder) {
  // Only allow different decoders if the expected decoder is expected to be
  // able to apply safety.
  bool test = (&decoder == &ExpectedDecoder());
  if (!test) {
    if (nacl_arm_dec::DECODER_ERROR != ExpectedDecoder().safety(inst)) {
      EXPECT_EQ(nacl_arm_dec::DECODER_ERROR, ExpectedDecoder().safety(inst))
          << "Expected " << ExpectedDecoder().name() << " but found "
          << decoder.name() << " for " << InstContents();
    }
  }
  // Only allow additional sanity checks if the found decoder is the
  // expected decoder.
  return test;
}

void DecoderTester::TestAtIndex(int index) {
  while (true) {
    // First see if we have completed generating test.
    if (index < 0) {
      ProcessMatch();
      return;
    }

    // If reached, more pattern to process.
    char selector = Pattern(index);

    if (selector == '0') {
      SetBit(index, false);
      --index;
      continue;
    }

    if (selector == '1') {
      SetBit(index, true);
      --index;
      continue;
    }

    if (islower(selector)) {
      // Find the length of the run of the same character (selector) in
      // the pattern. Then, test all combinations of zero and one for the
      // run length.
      int length = 1;
      while (Pattern(index - length) == selector) ++length;
      TestAtIndexExpandAll(index, length);
      return;
    }

    if (isupper(selector)) {
      // Find length of selector, then expand.
      int length = 1;
      while (Pattern(index - length) == selector) ++length;
      if (length <= 4) {
        TestAtIndexExpandAll(index, length);
      } else {
        TestAtIndexExpandFill4(index, length, true);
        TestAtIndexExpandFill4(index, length, false);
      }
      return;
    }
    GTEST_FAIL() << "Don't understand pattern character '" << selector << "'";
  }
}

void DecoderTester::TestAtIndexExpandAll(int index, int length) {
  if (length == 32) {
    // To be safe about overflow conditions, we use the recursive solution here.
    // Try setting index bit to false and continue testing.
    SetBit(index, false);
    TestAtIndexExpandAll(index-1, length-1);

    // Try setting index bit to true and continue testing.
    SetBit(index, true);
    TestAtIndexExpandAll(index-1, length-1);
  } else if (length == 0) {
    TestAtIndex(index);
  } else {
    // Use an (hopefully faster) iterative solution.
    uint32_t i;
    for (i = 0; i <= (((uint32_t) 1) << length) - 1; ++i) {
      SetBitRange(index - (length - 1), length, i);
      TestAtIndex(index - length);
    }
  }
}

void DecoderTester::FillBitRange(int index, int length, bool value) {
  if (length == 0)
    return;
  uint32_t val = (value ? 1 : 0);
  uint32_t bits = (val << length) - val;
  SetBitRange(index - (length - 1), length, bits);
}

void DecoderTester::TestAtIndexExpandFill(int index, int length, bool value) {
  FillBitRange(index, length, value);
  TestAtIndex(index - length);
}

void DecoderTester::TestAtIndexExpandFillAll(
    int index, int stride, int length, bool value) {
  ASSERT_LT(stride, 32) << "TestAtIndexExpandFillAll - illegal stride";
  if (stride == 0) {
    TestAtIndexExpandFill(index, length, value);
    return;
  } else {
    uint32_t i;
    for (i = 0; i <= (((uint32_t) 1) << stride) - 1; ++i) {
      SetBitRange(index - (stride - 1), stride, i);
      TestAtIndexExpandFill(index - stride, length - stride, value);
    }
  }
}

void DecoderTester::TestAtIndexExpandFill4(int index, int length, bool value) {
  int stride = 4;
  ASSERT_GT(length, stride);
  int window = length - stride;
  for (int i = 0; i <= window; ++i) {
    FillBitRange(index, i, value);
    TestAtIndexExpandFillAll(index - i, stride, length - i, value);
  }
}

Arm32DecoderTester::Arm32DecoderTester(
    const NamedClassDecoder& expected_decoder)
    : DecoderTester(),
      expected_decoder_(expected_decoder),
      pattern_(""),
      state_(),
      inst_((uint32_t) 0xFFFFFFFF) {
}

void Arm32DecoderTester::Test(const char* pattern) {
  pattern_ = pattern;
  ASSERT_EQ(static_cast<int>(kArm32InstSize),
            static_cast<int>(strlen(pattern_)))
      << "Arm32 pattern length incorrect: " << pattern_;
  TestAtIndex(kArm32InstSize-1);
}

const NamedClassDecoder& Arm32DecoderTester::ExpectedDecoder() const {
  return expected_decoder_;
}

static inline char BoolToChar(bool value) {
  return '0' + (uint8_t) value;
}

const char* Arm32DecoderTester::InstContents() const {
  static char buffer[kArm32InstSize + 1];
  for (int i = 1; i <= kArm32InstSize; ++i) {
    buffer[kArm32InstSize - i] = BoolToChar(inst_.Bit(i - 1));
  }
  buffer[kArm32InstSize] = '\0';
  return buffer;
}

char Arm32DecoderTester::Pattern(int index) const {
  return pattern_[kArm32InstSize - (index + 1)];
}

void Arm32DecoderTester::InjectInstruction(nacl_arm_dec::Instruction inst) {
  inst_.Copy(inst);
}

const NamedClassDecoder& Arm32DecoderTester::GetInstDecoder() const {
  return state_.decode_named(inst_);
}

void Arm32DecoderTester::ProcessMatch() {
  // Completed pattern, decode and test resulting state.
  const NamedClassDecoder& decoder = GetInstDecoder();
  if (!PassesParsePreconditions(inst_, decoder))
    return;
  if (MAY_BE_SAFE == decoder.safety(inst_))
    ApplySanityChecks(inst_, decoder);
}

void Arm32DecoderTester::SetBit(int index, bool value) {
  ASSERT_LT(index, kArm32InstSize)
      << "Arm32DecoderTester::SetBit(" << index << ", " << value << ")";
  ASSERT_GE(index , 0)
      << "Arm32DecoderTester::SetBit(" << index << ", " << value << ")";
  inst_.SetBit(index, value);
}

void Arm32DecoderTester::SetBitRange(int index, int length, uint32_t value) {
  ASSERT_LE(index + length, kArm32InstSize)
      << "Arm32DecoderTester::SetBitRange(" << index << ", " << length
      << ", " << value << ")";
  ASSERT_GE(index, 0)
      << "Arm32DecoderTester::SetBitRange(" << index << ", " << length
      << ", " << value << ")";
  inst_.SetBits(index + (length - 1), index, value);
}

}  // namespace nacl_arm_test
