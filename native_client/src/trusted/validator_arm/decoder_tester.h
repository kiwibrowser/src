/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Tests the decoder.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_DECODER_TESTER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_DECODER_TESTER_H_

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

#include "native_client/src/trusted/validator_arm/named_class_decoder.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_named_decoder.h"

namespace nacl_arm_test {

// Defines a decoder tester that enumerates an instruction pattern,
// and tests that all of the decoded patterns match the expected
// class decoder, and that any additional sanity checks, specific
// to the instruction apply.
//
// Patterns are sequences of characters as follows:
//   '1' - Bit must be value 1.
//   '0' - Bit must be value 0.
//   'aaa...aa' (for some sequence of m lower case letters) -
//       Try all possible combinations of bits for the m bytes.
//   'AAA...A'  (for some sequence of m upper case letters) -
//       Try the following combinations:
//         (1) All m bits set to 1.
//         (2) All m bits set to 0.
//         (3) For each 4-bit subsequence, try all combinations,
//             setting remaining bits to 1.
//         (4) For each 4-bit subsequence, try all combinations,
//             setting remaining bits to 0.
//
//  Bits are specified from the largest bit downto the smallest bit.
class DecoderTester {
 public:
  DecoderTester();
  virtual ~DecoderTester() {}

  // Runs any parse preconditions that should be applied to the test
  // pattern to determine if the pattern should be tested. This
  // virtual allows a hook to special case out what can't be described
  // using a single pattern string. Returns true if all preconditions
  // are met. Otherwise returns false. The default implementation
  // always returns true.
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);

  // Once an instruction is decoded, and the test pattern passes parse
  // preconditions, this method is called to apply sanity checks on
  // the matched decoder. The default checks that the expected class
  // name matches the name of the decoder. Returns whether further
  // checking of the instruction should be performed. In particular,
  // false is returned if a major problem was found, which will likely
  // cause other sanity checks to (possibly incorrectly) fail.
  virtual bool ApplySanityChecks(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);

  // Returns the expected decoder.
  virtual const NamedClassDecoder& ExpectedDecoder() const = 0;

  // Defines what should be done once a test pattern has been generated.
  virtual void ProcessMatch() = 0;

  // Allows the injection of an instruction. Used one to inject the instruction
  // that the subsequent call ProcessMatch will use.
  virtual void InjectInstruction(nacl_arm_dec::Instruction inst) = 0;

  // Runs the decoder on the current instruction and returns the
  // corresponding named decoder, as selected by the corresponding
  // decoder state.
  virtual const NamedClassDecoder& GetInstDecoder() const = 0;

 protected:
  // Returns a printable version of the contents of the tested instruction.
  // Used to print out useful test failures.
  // Note: This function may not be thread safe, and the result may
  // only be valid till the next call to this method.
  virtual const char* InstContents() const = 0;

  // Returns the character at the given index in the pattern that
  // should be tested.
  virtual char Pattern(int index) const = 0;

  // Conceptually sets the corresponding bit in the instruction.
  virtual void SetBit(int index, bool value) = 0;

  // Conceptually sets the corresponding sequence of bits in the
  // instruction to the given value.
  virtual void SetBitRange(int index, int length, uint32_t value) = 0;

  // Expands the pattern starting at the given index in the pattern
  // being expanded.
  void TestAtIndex(int index);

  // Expands the pattern starting at the given index, filling in all
  // possible combinations for the next length bits.
  void TestAtIndexExpandAll(int index, int length);

  // Expands the pattern starting at the given index, filling in the
  // next length bits with the given value.
  void TestAtIndexExpandFill(int index, int length, bool value);

  // Expands the pattern starting at the given index, filling the length
  // bits with each possible subpattern of four bits, surrounded by the
  // given value.
  void TestAtIndexExpandFill4(int index, int length, bool value);

  // Expands the pattern starting at the given index, filling the
  // next stride bits with all possible combinations of 0 and 1,
  // followed by the length-stride bits being set to the given value.
  // Note: Current implementation limits stride to less than 32.
  void TestAtIndexExpandFillAll(int index,
                                int stride, int length, bool Value);

  // Fills the next length bits with the corresponding value being repeated
  // length times.
  void FillBitRange(int index, int length, bool value);
};

// Helper macro for testing if preconditions are met within the
// ApplySanityChecks method of a DecoderTester. That is, if the
// precondition is not met, exit the routine and return false.
// Otherwise, the precondition of the remaining code has been met,
// and execution continues.
#define NC_PRECOND(test) \
  { if (!(test)) return false; }

// Helper macro for testing if an (error) precondition a != b is met
// within the ApplySanityChecks method of a DecoderTester. That is,
// if a == b, generate a gtest error and then stop the application
// from doing further checks for the given instruction.
#define NC_EXPECT_NE_PRECOND(a, b) \
  { EXPECT_NE(a, b) << InstContents(); \
    NC_PRECOND((a) != (b)); \
  }

// Helper macro for testing if an (error) precondition a == b is met
// within the ApplySanityChecks method of a DecoderTester. That is,
// if a != b, generate a gtest error and then stop the application
// from doing further checks for the given instruction.
#define NC_EXPECT_EQ_PRECOND(a, b) \
  { EXPECT_EQ(a, b) << InstContents(); \
    NC_PRECOND((a) == (b)); \
  }

// Helper macro for testing if an (error) precondition c is false
// withing the ApplySanityChecks method of a DecoderTester. That is,
// if !c, generate a gtest error and then stop the application from
// doing further checks for the given instruction.
#define NC_EXPECT_FALSE_PRECOND(a) \
  { EXPECT_FALSE(a) << InstContents(); \
    NC_PRECOND(!(a)); \
  }

// Defines a decoder tester that enumerates an Arm32 instruction pattern,
// and tests that all of the decoded patterns match the expected class
// decoder, and that any additional sanity checks, specific to the
// instruction apply.
//
// Note: Patterns must be of length 32.
class Arm32DecoderTester : public DecoderTester {
 public:
  explicit Arm32DecoderTester(
      const NamedClassDecoder& expected_decoder);
  void Test(const char* pattern);
  virtual const NamedClassDecoder& ExpectedDecoder() const;
  virtual void ProcessMatch();
  virtual void InjectInstruction(nacl_arm_dec::Instruction inst);
  virtual const NamedClassDecoder& GetInstDecoder() const;

 protected:
  virtual const char* InstContents() const;
  virtual char Pattern(int index) const;
  virtual void SetBit(int index, bool value);
  virtual void SetBitRange(int index, int length, uint32_t value);

  // The expected decoder class.
  const NamedClassDecoder& expected_decoder_;

  // The pattern being enumerated.
  const char* pattern_;

  // The decoder to use.
  NamedArm32DecoderState state_;

  // The instruction currently being enumerated.
  nacl_arm_dec::Instruction inst_;
};

}  // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_DECODER_TESTER_H_
