/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

/*
 * Small unit tests for the ARM validator
 *
 * Also see validator_large_tests.cc,
 * and validator_tests.h for the testing infrastructure.
 */

#include "native_client/src/trusted/validator_arm/validator_tests.h"

using nacl_val_arm_test::ProblemRecord;
using nacl_val_arm_test::ProblemSpy;
using nacl_val_arm_test::ValidatorTests;
using nacl_val_arm_test::kDefaultBaseAddr;
using nacl_val_arm_test::kBytesPerBundle;
using nacl_val_arm_test::kCodeRegionSize;
using nacl_val_arm_test::kDataRegionSize;
using nacl_val_arm_test::kAbiReadOnlyRegisters;
using nacl_val_arm_test::kAbiDataAddrRegisters;
using nacl_val_arm_test::arm_inst;
using nacl_arm_dec::kArm32InstSize;
using nacl_arm_dec::kNop;
using nacl_arm_dec::kLiteralPoolHead;
using nacl_arm_dec::kFailValidation;
using nacl_arm_dec::Instruction;
using nacl_arm_dec::ClassDecoder;

namespace {

const arm_inst kSvc0 = 0xEF000000;  // SVC #0

// Holds instruction and message to print if there is an issue when it
// is tested.
struct AnnotatedInstruction {
  arm_inst inst;
  const char *about;
};

arm_inst ChangeInstCond(Instruction inst, Instruction::Condition cond) {
  return ValidatorTests::ChangeCond(inst.Bits(), cond);
}

// Tests if a list of instructions generate the same dynamic code
// replacement sentinel.
void test_if_dynamic_code_replacement_sentinels_match(
    const ValidatorTests* tester,
    const AnnotatedInstruction* insts,
    size_t num_insts,
    arm_inst expected_sentinel) {
  const Instruction expected_inst(expected_sentinel);

  // Try each instruction.
  for (size_t i = 0; i < num_insts; i++) {
    Instruction test_inst(insts[i].inst);

    // Try each possible condition (conditions shouldn't affect this virtual).
    for (Instruction::Condition cond = Instruction::EQ;
         cond < Instruction::UNCONDITIONAL;
         cond = Instruction::Next(cond)) {
      Instruction test(ChangeInstCond(test_inst, cond));
      Instruction expected(ChangeInstCond(expected_inst, cond));

      const ClassDecoder& decoder = tester->decode(test);
      Instruction sentinel(decoder.dynamic_code_replacement_sentinel(test));
      EXPECT_TRUE(expected.Equals(sentinel)) <<
        std::hex << test.Bits() << "->" <<
          std::hex << sentinel.Bits() << " != " <<
          std::hex << expected.Bits() << ": " << insts[i].about;
    }
  }
}

// Tests if a list of instructions generate the same dynamic code replacement
// sentinel.
void test_if_dynamic_code_replacement_sentinels_unchanged(
    const ValidatorTests* tester,
    const AnnotatedInstruction* insts,
    size_t num_insts) {
  // Try each instruction.
  for (size_t i = 0; i < num_insts; i++) {
    Instruction test_inst(insts[i].inst);

    // Try each possible condition (conditions shouldn't affect this virtual).
    for (Instruction::Condition cond = Instruction::EQ;
         cond < Instruction::UNCONDITIONAL;
         cond = Instruction::Next(cond)) {
      Instruction test(ChangeInstCond(test_inst, cond));
      const ClassDecoder& decoder = tester->decode(test);
      Instruction sentinel(decoder.dynamic_code_replacement_sentinel(test));
      EXPECT_TRUE(test.Equals(sentinel)) <<
          std::hex << test.Bits() << "->" <<
          std::hex << sentinel.Bits() << ": " << insts[i].about;
    }
  }
}

// Test that the old code can be replaced by the new code.
void TestDynamicCodeReplacement(
    SfiValidator *validator,
    bool expected_result,
    const vector<arm_inst>& old_code,
    const vector<arm_inst>& new_code,
    const char *msg) {
  EXPECT_EQ(old_code.size(), new_code.size());

  size_t size = old_code.size() * kArm32InstSize / CHAR_BIT;
  CodeSegment new_segment((uint8_t *) &new_code[0], kDefaultBaseAddr, size);
  CodeSegment old_segment((uint8_t *) &old_code[0], kDefaultBaseAddr, size);

  EXPECT_EQ(expected_result,
            validator->ValidateSegmentPair(old_segment, new_segment, NULL)) <<
      "Code replacement expected different validation result for " << msg;
}

// Verify that there is enough room in ViolationSet to include all
// elements in enum Violation.
TEST_F(ValidatorTests, ViolationsFitInViolationSet) {
  EXPECT_LT(static_cast<size_t>(nacl_arm_dec::OTHER_VIOLATION),
            sizeof(nacl_arm_dec::ViolationSet) * CHAR_BIT);
}

TEST_F(ValidatorTests, NopBundle) {
  vector<arm_inst> code(_validator->InstructionsPerBundle(), kNop);
  validation_should_pass(&code[0], code.size(), kDefaultBaseAddr,
                         "NOP bundle");
}

/*
 * Primitive tests checking various constructor properties.  Any of these
 * failing would be a very bad sign indeed.
 */

TEST_F(ValidatorTests, RecognizesDataAddressRegisters) {
  // Note that the logic below needs to be kept in sync with the definition
  // of kAbiDataAddrRegisters at the top of this file.
  //
  // This test is pretty trivial -- we can exercise the data_address_register
  // functionality more deeply with pattern tests below.
  for (int i = 0; i < 16; i++) {
    Register r(i);
    if (r.Equals(nacl_arm_dec::Register::Sp())) {
      EXPECT_TRUE(_validator->is_data_address_register(r))
          << "Stack pointer must be a data address register.";
    } else {
      EXPECT_FALSE(_validator->is_data_address_register(r))
          << "Only the stack pointer must be a data address register.";
    }
  }
}

TEST_F(ValidatorTests, GeneratesCorrectMasksFromSizes) {
  EXPECT_EQ(0xC0000000, _validator->data_address_mask());
  EXPECT_EQ(0xE000000F, _validator->code_address_mask());

  // Reinitialize the validator to test a different bundle size.
  SfiValidator *new_validator = new SfiValidator(32,
                                                 kCodeRegionSize,
                                                 kDataRegionSize,
                                                 kAbiReadOnlyRegisters,
                                                 kAbiDataAddrRegisters,
                                                 _validator->CpuFeatures());
  delete _validator;
  _validator = new_validator;

  EXPECT_EQ(0xC0000000, _validator->data_address_mask())
      << "Changes in bundle size should not affect the data mask.";
  EXPECT_EQ(0xE000001F, _validator->code_address_mask())
      << "Changes in bundle size must affect the code mask.";
}

// Code validation tests

TEST_F(ValidatorTests, DirectBranchTargetCalculation) {
  const nacl_arm_dec::Arm32DecoderState decode_state;
  // Try decoding PC-relative branches from interesting PCs.
  uint32_t start_addrs[] = { 0x00000000, 0x00010000, 0x00020000, 0x06789abc,
                             0x12345678, 0x3fffffff, 0xdeadbeef, 0xff000000, };
  for (size_t a = 0; a < NACL_ARRAY_SIZE(start_addrs); ++a) {
    uint32_t start_addr = start_addrs[a];
    // All PC-relative branches supported by NaCl.
    arm_inst insts[] = {
      0xea000000,  // B PC+8+0
      0xeb000000,  // BL PC+8+0
    };
    for (arm_inst* inst = insts;
         inst < insts + NACL_ARRAY_SIZE(insts);
         ++inst) {
      for (Instruction::Condition cond = Instruction::EQ;
           cond <= Instruction::AL;
           cond = Instruction::Next(cond)) {
        *inst = ChangeCond(*inst, cond);
        for (int32_t imm = -2050; imm < 2050; ++imm) {
          // For all PC-relative branches that NaCl supports:
          //   imm24(23:0);
          //   imm32 = SignExtend(imm24:'00', 32);
          //   targetAddress = Align(PC,4) + imm32;
          // Where:
          //   The PC value of an instruction is its address plus 8 for an
          //   ARM instruction. The Align(PC, 4) value of an instruction is
          //   its PC value ANDed with 0xFFFFFFFC to force it to be
          //   word-aligned. There is no difference between the PC and
          //   Align(PC, 4) values for an ARM instruction.
          *inst = (*inst & 0xff000000) | ((imm >> 2) & 0x00ffffff);
          const uint8_t *bytes = reinterpret_cast<const uint8_t *>(inst);
          CodeSegment segment(bytes, start_addr, sizeof(arm_inst));
          nacl_arm_val::DecodedInstruction inst(start_addr, segment[start_addr],
                                                decode_state.decode(
                                                    segment[start_addr]));
          EXPECT_TRUE(inst.is_relative_branch());
          EXPECT_EQ(start_addr + 8 + (imm & 0xfffffffc), inst.branch_target());
        }
      }
    }
  }
}

// Here are examples of every form of safe store permitted in a Native Client
// program.  These stores have common properties:
//  1. The high nibble is 0, to allow tests to write an arbitrary predicate.
//  2. They address memory only through r1 (assumed to be the first register
//     in the about text).
//  3. They do not do anything dumb, like try to alter SP or PC.
static const AnnotatedInstruction examples_of_safe_stores[] = {
  // Single-register stores
  { 0x05810000, "str r0, [r1]: simple no-displacement store" },
  { 0x05810123, "str r0, [r1, #0x123]: positive displacement" },
  { 0x05010123, "str r0, [r1, #-0x123]: negative displacement" },
  { 0x05A10123, "str r0, [r1, #0x123]!: positive disp + writeback" },
  { 0x05210123, "str r0, [r1, #-0x123]!: negative disp + writeback" },
  { 0x04810123, "str r0, [r1], #0x123: positive post-indexing" },
  { 0x04010123, "str r0, [r1], #-0x123: negative post-indexing" },
  { 0x06810002, "str r0, [r1], r2: positive register post-indexing" },
  { 0x06010002, "str r0, [r1], -r2: negative register post-indexing" },

  // Two-register store
  { 0x01C120F0, "strd r2, r3, [r1]: basic 64-bit store" },
  { 0x01C124F2, "strd r2, r3, [r1, #42]: positive disp 64-bit store" },
  { 0x014124F2, "strd r2, r3, [r1, #-42]: negative disp 64-bit store" },
  { 0x01E124F2, "strd r2, r3, [r1, #42]!: positive disp 64-bit store + wb" },
  { 0x016124F2, "strd r2, r3, [r1, #-42]!: negative disp 64-bit store + wb" },
  { 0x00C124F2, "strd r2, r3, [r1], #42: post-inc 64-bit store" },
  { 0x004124F2, "strd r2, r3, [r1], #-42: post-dec 64-bit store" },

  // Store-exclusive
  { 0x01810F92, "strex r0, r2, [r1]: store exclusive" },

  // Store-multiple
  // Note: It is illegal to store r9 (NaCl constraint), so it must
  // be ommitted from the register sets.
  { 0x0881FDFF, "stm r1, { r0-r8, r10-r15 }: store multiple, no writeback" },
  // Note: can't store registers whose number is less than Rt when there is
  //       writeback. E.g. stm r1! { r0-r15 } is unknown.
  { 0x08A1FDFE, "stm r1!, { r1-r8, r10-r15 }: store multiple, writeback" },
  { 0x08A1FDFC, "stm r1!, { r2-r8, r10-r15 }: store multiple, writeback" },

  // Stores from the floating point / vector register file
  // These all compile to STC instructions.
  { 0x0D810B00, "vstr d0, [r1]: direct vector store" },
  { 0x0D810B99, "vstr d0, [r1, #0x99]: positive displacement vector store" },
  { 0x0D010B99, "vstr d0, [r1, #-0x99]: negative displacement vector store" },
  { 0x0C810B10, "vstmia r1, { d0-d7 }: no writeback" },
  { 0x0CA10B10, "vstmia r1!, { d0-d7 }: writeback" },
};

// Note: We assume that r3 is used for the operand, if it isn't the same
// as the destination (i.e. r1).
static const AnnotatedInstruction examples_of_safe_masks[] = {
  { 0x03C11103, "bic r1, r1, #0xC0000000: simple in-place mask (form 1)" },
  { 0x03C114C0, "bic r1, r1, #0xC0000000: simple in-place mask (form 2)" },
  { 0x03C314C0, "bic r1, r3, #0xC0000000: mask with register move" },
  { 0x03C114FF, "bic r1, r1, #0xFF000000: overzealous but correct mask" },
};

TEST_F(ValidatorTests, SafeMaskedStores) {
  // Produces many examples of masked stores using the safe store table (above)
  // and the list of possible masking instructions (below).
  //
  // Each mask instruction must leave a valid (data) address in r1.
  for (unsigned p = 0; p < 15; p++) {
    // Conditionally executed instructions have a top nibble of 0..14.
    // 15 is an escape sequence used to fit in additional encodings.
    arm_inst predicate = p << 28;

    for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks); m++) {
      for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stores); s++) {
        ostringstream message;
        message << examples_of_safe_masks[m].about
                << ", "
                << examples_of_safe_stores[s].about
                << " (predicate #" << p << ")";
        arm_inst program[] = {
          examples_of_safe_masks[m].inst | predicate,
          examples_of_safe_stores[s].inst | predicate,
        };
        validation_should_pass2(program,
                                2,
                                kDefaultBaseAddr,
                                message.str());
      }
    }
  }
}

static const AnnotatedInstruction examples_of_safe_unconditional_stores[] = {
  // Vector stores
  { 0xF481A5AF, "vst2.16 {d10[2],d12[2]}, [r1]: simple vector store" },
  { 0xF401A64F, "vst1.16 {d10-d12}, [r1]: larger vector store" },
  { 0xF4010711, "vst1.8 {d0}, [r1, :64], r1: register post-increment" },
};

TEST_F(ValidatorTests, SafeUnconditionalMaskedStores) {
  // Produces many examples of unconditional masked stores using the safe
  // unconditional store table (above) and the list of possible masking
  // instructions (below).
  //
  // Each mask instruction must leave a valid (data) address in r1.

  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks); m++) {
    for (unsigned s = 0;
         s < NACL_ARRAY_SIZE(examples_of_safe_unconditional_stores);
         s++) {
      ostringstream message;
      message << examples_of_safe_masks[m].about
              << ", "
              << examples_of_safe_unconditional_stores[s].about;
      arm_inst program[] = {
        ValidatorTests::ChangeCond(examples_of_safe_masks[m].inst,
                                   Instruction::AL),
        examples_of_safe_unconditional_stores[s].inst,
      };
      validation_should_pass2(program,
                              2,
                              kDefaultBaseAddr,
                              message.str());
    }
  }
}

TEST_F(ValidatorTests, SafeConditionalStores) {
  // Produces many examples of conditional stores using the safe store table
  // (above) and the list of possible conditional guards (below).
  //
  // Each conditional guard must set the Z flag iff r1 contains a valid address.
  static const AnnotatedInstruction guards[] = {
    { 0x03110103, "tst r1, #0xC0000000: precise guard, GCC encoding" },
    { 0x031104C0, "tst r1, #0xC0000000: precise guard, alternative encoding" },
    { 0x031101C3, "tst r1, #0xF0000000: overzealous (but correct) guard" },
  };

  // Reinitialize the validator to test both allowing and disallowing
  // conditional memory accesses.
  for (int allow_cond = 0; allow_cond <= 1; ++allow_cond) {
    NaClCPUFeaturesArm cpu_features;
    NaClCopyCPUFeaturesArm(&cpu_features, _validator->CpuFeatures());
    NaClSetCPUFeatureArm(&cpu_features, NaClCPUFeatureArm_CanUseTstMem,
                         allow_cond);
    delete _validator;
    _validator = new SfiValidator(kBytesPerBundle,
                                  kCodeRegionSize,
                                  kDataRegionSize,
                                  kAbiReadOnlyRegisters,
                                  kAbiDataAddrRegisters,
                                  &cpu_features);

    // Currently we only support *unconditional* conditional stores.
    // Meaning the guard is unconditional and the store is if-equal.
    //   e.g.: tst.al rbase, #mask
    //         str.eq rval, [rbase]
    for (Instruction::Condition guard_cond = Instruction::EQ;
         guard_cond <= Instruction::AL;
         guard_cond = Instruction::Next(guard_cond)) {
      for (Instruction::Condition store_cond = Instruction::EQ;
           store_cond <= Instruction::AL;
           store_cond = Instruction::Next(store_cond)) {
        for (size_t guard = 0;
             guard < NACL_ARRAY_SIZE(guards);
             guard++) {
          arm_inst guard_inst = guards[guard].inst;
          for (int use_same_base = 0; use_same_base <= 1; ++use_same_base) {
            // Alternate between using the same rbase for tst and str.
            // TODO(jfb) This is hackish, and should be better handled
            //           throughout these tests. AnnotatedInstruction is
            //           probably the right place for this.
            unsigned base = use_same_base ? 1 : 2;  // Rn is now r1 or r2.
            guard_inst &= 0xFFF0FFFF;  // Rn is at (19:16).
            guard_inst |= base << 16;
            for (size_t store = 0;
                 store < NACL_ARRAY_SIZE(examples_of_safe_stores);
                 store++) {
              arm_inst store_inst = examples_of_safe_stores[store].inst;
              ostringstream message;
              message << guards[guard].about << " with condition "
                      << Instruction::ToString(guard_cond)
                      << (use_same_base ?
                          " and same base registers," :
                          " and different base registers,")
                      << " followed by "
                      << examples_of_safe_stores[store].about
                      << " with condition "
                      << Instruction::ToString(store_cond);
              arm_inst program[] = {
                ValidatorTests::ChangeCond(guard_inst, guard_cond),
                ValidatorTests::ChangeCond(store_inst, store_cond),
              };
              if (allow_cond &&
                  use_same_base &&
                  (guard_cond == Instruction::AL) &&
                  (store_cond == Instruction::EQ)) {
                validation_should_pass2(program,
                                        2,
                                        kDefaultBaseAddr,
                                        message.str());
              } else {
                validation_should_fail(program,
                                       2,
                                       kDefaultBaseAddr,
                                       message.str());
              }
            }
          }
        }
      }
    }
  }
}

static const AnnotatedInstruction examples_of_invalid_masks[] = {
  { 0x01A01003, "mov r1, r3: not even a mask" },
  { 0x03C31000, "bic r1, r3, #0: doesn't mask anything" },
  { 0x03C31102, "bic r1, r3, #0x80000000: doesn't mask enough bits" },
  { 0x03C311C1, "bic r1, r3, #0x70000000: masks the wrong bits" },
};

TEST_F(ValidatorTests, InvalidMasksOnSafeStores) {
  for (unsigned p = 0; p < 15; p++) {
    // Conditionally executed instructions have a top nibble of 0..14.
    // 15 is an escape sequence used to fit in additional encodings.
    arm_inst predicate = p << 28;

    for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_invalid_masks); m++) {
      for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stores); s++) {
        ostringstream message;
        message << examples_of_invalid_masks[m].about
                << ", "
                << examples_of_safe_stores[s].about
                << " (predicate #" << p << ")";
        arm_inst program[] = {
          examples_of_invalid_masks[m].inst | predicate,
          examples_of_safe_stores[s].inst | predicate,
        };

        ProblemSpy spy;
        validation_should_fail(program,
                               NACL_ARRAY_SIZE(program),
                               kDefaultBaseAddr,
                               message.str(),
                               &spy);
        vector<ProblemRecord> problems = spy.get_problems();

        // EXPECT/continue rather than ASSERT so that we run the other cases.
        EXPECT_EQ(1U, problems.size());
        if (problems.size() != 1) continue;

        ProblemRecord first = problems[0];
        EXPECT_EQ(kDefaultBaseAddr + 4, first.vaddr())
            << "Problem report must point to the store: "
            << message.str();
        EXPECT_FALSE(nacl_arm_dec::IsSafetyViolation(first.violation()))
            << "Store should not be unsafe even though the mask is bogys:"
            << message.str();
        EXPECT_EQ(nacl_arm_dec::LOADSTORE_VIOLATION, first.violation())
            << message.str();
      }
    }
  }
}

TEST_F(ValidatorTests, InvalidGuardsOnSafeStores) {
  static const AnnotatedInstruction invalid_guards[] = {
    { 0x03110100, "tst r1, #0: always sets Z" },
    { 0x03110102, "tst r1, #0x80000000: doesn't test enough bits" },
    { 0x031101C1, "tst r1, #0x70000000: doesn't test the right bits" },
    { 0x01A01003, "mov r1, r3: not even a test" },
    { 0x03310103, "teq r1, #0xC0000000: does the inverse of what we want" },
    { 0x03510103, "cmp r1, #0xC0000000: does the inverse of what we want" },
  };

  // We don't currently support conditional versions of the conditional guard.
  //
  // TODO(cbiffle): verify this in the test
  static const arm_inst guard_predicate = 0xE0000000;  // unconditional
  static const arm_inst store_predicate = 0x00000000;  // if-equal

  for (unsigned m = 0; m < NACL_ARRAY_SIZE(invalid_guards); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stores); s++) {
      ostringstream message;
      message << invalid_guards[m].about
              << ", "
              << examples_of_safe_stores[s].about;
      arm_inst program[] = {
        invalid_guards[m].inst | guard_predicate,
        examples_of_safe_stores[s].inst | store_predicate,
      };

      ProblemSpy spy;
      validation_should_fail(program,
                             NACL_ARRAY_SIZE(program),
                             kDefaultBaseAddr,
                             message.str(),
                             &spy);
      vector<ProblemRecord> problems = spy.get_problems();

      // EXPECT/continue rather than ASSERT so that we run the other cases.
      EXPECT_EQ(1U, problems.size());
      if (problems.size() != 1) continue;

      ProblemRecord first = problems[0];
      EXPECT_EQ(kDefaultBaseAddr + 4, first.vaddr())
          << "Problem report must point to the store: "
          << message.str();
      EXPECT_FALSE(nacl_arm_dec::IsSafetyViolation(first.violation()))
          << "Store should not be unsafe even though guard is bogus:"
          << message;
      EXPECT_EQ(nacl_arm_dec::LOADSTORE_VIOLATION, first.violation())
          << message;
    }
  }
}

TEST_F(ValidatorTests, ValidMasksOnUnsafeStores) {
  static const AnnotatedInstruction invalid_stores[] = {
    { 0x07810002, "str r0, [r1, r2]: register-plus-register addressing" },
    { 0x07010002, "str r0, [r1, -r2]: register-minus-register addressing" },
    { 0x07810182, "str r0, [r1, r2, LSL #3]: complicated addressing 1" },
    { 0x07018482, "str r0, [r1, -r2, ASR #16]: complicated addressing 2" },
  };

  for (unsigned p = 0; p < 15; p++) {
    // Conditionally executed instructions have a top nibble of 0..14.
    // 15 is an escape sequence used to fit in additional encodings.
    arm_inst predicate = p << 28;

    for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks); m++) {
      for (unsigned s = 0; s < NACL_ARRAY_SIZE(invalid_stores); s++) {
        ostringstream message;
        message << examples_of_safe_masks[m].about
                << ", "
                << invalid_stores[s].about
                << " (predicate #" << p << ")";
        arm_inst program[] = {
          examples_of_safe_masks[m].inst | predicate,
          invalid_stores[s].inst | predicate,
        };

        ProblemSpy spy;
        validation_should_fail(program,
                               NACL_ARRAY_SIZE(program),
                               kDefaultBaseAddr,
                               message.str(),
                               &spy);
        vector<ProblemRecord> problems = spy.get_problems();

        // EXPECT/continue rather than ASSERT so that we run the other cases.
        EXPECT_EQ(1U, problems.size());
        if (problems.size() != 1) continue;

        ProblemRecord first = problems[0];
        EXPECT_EQ(kDefaultBaseAddr + 4, first.vaddr())
            << "Problem report must point to the store: "
            << message.str();
        EXPECT_TRUE(nacl_arm_dec::IsSafetyViolation(first.violation()));
      }
    }
  }
}

TEST_F(ValidatorTests, ScaryUndefinedInstructions) {
  // These instructions are undefined today (ARMv7-A) but may become defined
  // tomorrow.  We ban them since we can't reason about their side effects.
  static const AnnotatedInstruction undefined_insts[] = {
    { 0xE05DEA9D, "An undefined instruction in the multiply space" },
  };
  for (unsigned i = 0; i < NACL_ARRAY_SIZE(undefined_insts); i++) {
    arm_inst program[] = { undefined_insts[i].inst };

    ProblemSpy spy;
    validation_should_fail(program,
                           NACL_ARRAY_SIZE(program),
                           kDefaultBaseAddr,
                           undefined_insts[i].about,
                           &spy);
    vector<ProblemRecord> problems = spy.get_problems();

    // EXPECT/continue rather than ASSERT so that we run the other cases.
    EXPECT_EQ(1U, problems.size());
    if (problems.size() != 1) continue;

    ProblemRecord first = problems[0];
    EXPECT_EQ(kDefaultBaseAddr, first.vaddr())
        << "Problem report must point to the only instruction: "
        << undefined_insts[i].about;
    EXPECT_TRUE(nacl_arm_dec::IsSafetyViolation(first.violation()))
        << "Instruction must be flagged by the decoder as unsafe:"
        << undefined_insts[i].about;
    EXPECT_EQ(nacl_arm_dec::UNDEFINED_VIOLATION, first.violation())
        << "Instruction must be flagged as UNDEFINED:"
        << undefined_insts[i].about;
  }
}

TEST_F(ValidatorTests, PcRelativeFirstInst) {
  // Note: This tests the fix for issue 2771.
  static const arm_inst pcrel_boundary_tests[] = {
    0xe59f0000,  // ldr     r0, [pc, #0]
    kNop,
    kNop,
    kNop,
  };
  validation_should_pass(pcrel_boundary_tests,
                         NACL_ARRAY_SIZE(pcrel_boundary_tests),
                         kDefaultBaseAddr,
                         "pc relative first instruction in first bundle");
}

TEST_F(ValidatorTests, PcRelativeFirst2ndBundle) {
  // Note: This tests the fix for issue 2771.
  static const arm_inst pcrel_boundary_tests[] = {
    kNop,
    kNop,
    kNop,
    kNop,
    0xe59f0000,  // ldr     r0, [pc, #0]
  };
  validation_should_pass(pcrel_boundary_tests,
                         NACL_ARRAY_SIZE(pcrel_boundary_tests),
                         kDefaultBaseAddr,
                         "pc relative first instruction in 2nd bundle");
}

TEST_F(ValidatorTests, SafeConditionalBicLdrTest) {
  // Test if we fixed bug with conditional Bic Loads (issue 2769).
  static const arm_inst bic_ldr_safe_test[] = {
    0x03c22103,  // biceq   r2, r2, #-1073741824    ; 0xc0000000
    0x01920f9f,  // ldrexeq r0, [r2]
  };
  validation_should_pass(bic_ldr_safe_test,
                         NACL_ARRAY_SIZE(bic_ldr_safe_test),
                         kDefaultBaseAddr,
                         "Safe conditional bic ldr test");
}

TEST_F(ValidatorTests, ConditionalBicsLdrTest) {
  // Test if we fail because Bic updates the flags register, making
  // the conditional Bic load incorrect (issue 2769).
  static const arm_inst bics_ldr_unsafe_test[] = {
    0x03d22103,  // bicseq  r2, r2, #-1073741824    ; 0xc0000000
    0x01920f9f,  // ldrexeq r0, [r2]
  };

  ProblemSpy spy;
  validation_should_fail(bics_ldr_unsafe_test,
                         NACL_ARRAY_SIZE(bics_ldr_unsafe_test),
                         kDefaultBaseAddr,
                         "Conditional bics ldr test",
                         &spy);
  vector<ProblemRecord> problems = spy.get_problems();

  EXPECT_EQ(1U, problems.size());
  if (problems.size() == 0) return;

  ProblemRecord problem = problems[0];
  EXPECT_EQ(kDefaultBaseAddr + 4, problem.vaddr())
      << "Problem report should point to the ldr instruction.";
  EXPECT_FALSE(nacl_arm_dec::IsSafetyViolation(problem.violation()));
  EXPECT_EQ(nacl_arm_dec::LOADSTORE_VIOLATION, problem.violation());
}

TEST_F(ValidatorTests, DifferentConditionsBicLdrTest) {
  // Test if we fail because the Bic and Ldr instructions have
  // different conditional flags.
  static const arm_inst bic_ldr_diff_conds[] = {
    0x03c22103,  // biceq   r2, r2, #-1073741824    ; 0xc0000000
    0xc1920f9f,  // ldrexgt r0, [r2]
  };

  ProblemSpy spy;
  validation_should_fail(bic_ldr_diff_conds,
                         NACL_ARRAY_SIZE(bic_ldr_diff_conds),
                         kDefaultBaseAddr,
                         "Different conditions bic ldr test",
                         &spy);
  vector<ProblemRecord> problems = spy.get_problems();

  EXPECT_EQ(1U, problems.size());
  if (problems.size() == 0) return;

  ProblemRecord problem = problems[0];
  EXPECT_EQ(kDefaultBaseAddr + 4, problem.vaddr())
      << "Problem report should point to the ldr instruction.";
  EXPECT_FALSE(nacl_arm_dec::IsSafetyViolation(problem.violation()));
  EXPECT_EQ(nacl_arm_dec::LOADSTORE_VIOLATION, problem.violation());
}

TEST_F(ValidatorTests, BfcLdrInst) {
  static const arm_inst bfc_inst[] = {
    0xe7df2f1f,  // bfc r2, #30, #2
    0xe1920f9f,  // ldrex r0, [r2]
  };
  validation_should_fail(bfc_inst,
                         NACL_ARRAY_SIZE(bfc_inst),
                         kDefaultBaseAddr,
                         "Bfc Ldr instruction mask has a mask that's "
                         "technically safe, but we disallow it out of "
                         "paranoia");
}

TEST_F(ValidatorTests, BfcLdrInstMaskTooBigTest) {
  static const arm_inst bfc_inst[] = {
    0xe7df2e9f,  // bfc r2, #29, #3
    0xe1920f9f,  // ldrex r0, [r2]
  };
  validation_should_fail(bfc_inst,
                         NACL_ARRAY_SIZE(bfc_inst),
                         kDefaultBaseAddr,
                         "Bfc Ldr instruction mask has a mask that's "
                         "technically safe (even too wide!), but we "
                         "disallow it out of paranoia");
}

TEST_F(ValidatorTests, BfcLdrInstMaskWrongPlaceTest) {
  static const arm_inst bfc_inst[] = {
    0xe7da2c9f,  // bfc r2, #25, #2
    0xe1920f9f,  // ldrex r0, [r2]
  };
  validation_should_fail(bfc_inst,
                         NACL_ARRAY_SIZE(bfc_inst),
                         kDefaultBaseAddr,
                         "Bfc Ldr instruction mask has a mask that's "
                         "not safe, and anyways we disallow it out of "
                         "paranoia");
}

// Test effects of virtual dynamic_code_replacement_sentinel on the movw
// instruction.
TEST_F(ValidatorTests, DynamicCodeReplacementSentinelMovw) {
  // Test cases where the sentinel changes for movw.
  const AnnotatedInstruction inst[] = {
    {0xe30a3aaa, "movw    r3, #43690      ; 0xaaaa"},
    {0xe3053555, "movw    r3, #21845      ; 0x5555"},
  };

  // Test cases where the sentinel doesn't change for movw.
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      inst, NACL_ARRAY_SIZE(inst), 0xe3003000);
  const AnnotatedInstruction unchanged[] = {
    // If already the sentinel, nothing changes.
    {0xe3003000, "movw    r3, #0          ; 0x0000"},
    // Note: These instructions may not validate for other reasons,
    // but we are only testing the virtual
    // dynamic_code_replacement_sentinel, and that it doesn't
    // apply changes when the register is in {r9, sp, lr, pc}.
    {0xe3059555, "movw    r9, #21845      ; 0x5555"},
    {0xe305d555, "movw    sp, #21845      ; 0x5555"},
    {0xe305e555, "movw    lr, #21845      ; 0x5555"},
    {0xe305f555, "movw    pc, #21845      ; 0x5555"},
  };
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      unchanged, NACL_ARRAY_SIZE(unchanged));
}

// Test effects of virtual dynamic_code_replacement_sentinel on the movt
// instruction.
TEST_F(ValidatorTests, DynamicCodeReplacementSentinelMovt) {
  // Test cases where the sentinel changes for movt.
  const AnnotatedInstruction inst[] = {
    {0xe34a5aaa, "movt    r5, #43690      ; 0xaaaa"},
    {0xe3455555, "movt    r5, #21845      ; 0x5555"},
  };
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      inst, NACL_ARRAY_SIZE(inst), 0xe3405000);

  // Test cases where the sentinel doesn't change for movt.
  const AnnotatedInstruction unchanged[] = {
    // If already the sentinel, nothing changes.
    {0xe3405000, "movt    r5, #0          ; 0x0000"},
    // Note: These instructions may not validate for other reasons,
    // but we are only testing the virtual
    // dynamic_code_replacement_sentinel, and that it doesn't
    // apply changes when the register is in {r9, sp, lr, pc}.
    {0xe3459555, "movt    r9, #21845      ; 0x5555"},
    {0xe345d555, "movt    sp, #21845      ; 0x5555"},
    {0xe345e555, "movt    lr, #21845      ; 0x5555"},
    {0xe345f555, "movt    pc, #21845      ; 0x5555"},
  };
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      unchanged, NACL_ARRAY_SIZE(unchanged));
}

// Test effects of virtual dynamic_code_replacement_sentinel on the orr
// instruction.
TEST_F(ValidatorTests, DynamicCodeReplacementSentinelOrr) {
  // Test cases where the sentinel changes for orr.
  const AnnotatedInstruction orr[] = {
    {0xe38454aa, "orr     r5, r4, #-1442840576    ; 0xaa000000"},
    {0xe38458aa, "orr     r5, r4, #11141120       ; 0xaa0000"},
    {0xe3845caa, "orr     r5, r4, #43520  ; 0xaa00"},
    {0xe38450aa, "orr     r5, r4, #170    ; 0xaa"},
    {0xe3845455, "orr     r5, r4, #1426063360     ; 0x55000000"},
    {0xe3845855, "orr     r5, r4, #5570560        ; 0x550000"},
    {0xe3845c55, "orr     r5, r4, #21760  ; 0x5500"},
    {0xe3845055, "orr     r5, r4, #85     ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      orr, NACL_ARRAY_SIZE(orr), 0xe3845000);
  const AnnotatedInstruction orrs[] = {
    {0xe39454aa, "orrs    r5, r4, #-1442840576    ; 0xaa000000"},
    {0xe39458aa, "orrs    r5, r4, #11141120       ; 0xaa0000"},
    {0xe3945caa, "orrs    r5, r4, #43520  ; 0xaa00"},
    {0xe39450aa, "orrs    r5, r4, #170    ; 0xaa"},
    {0xe3945455, "orrs    r5, r4, #1426063360     ; 0x55000000"},
    {0xe3945855, "orrs    r5, r4, #5570560        ; 0x550000"},
    {0xe3945c55, "orrs    r5, r4, #21760  ; 0x5500"},
    {0xe3945055, "orrs    r5, r4, #85     ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      orrs, NACL_ARRAY_SIZE(orrs), 0xe3945000);

  // Test cases where the sentinel doesn't change for orr.
  const AnnotatedInstruction unchanged[] = {
    // Note: These instructions may not validate for other reasons,
    // but we are only testing the virtual
    // dynamic_code_replacement_sentinel, and that it doesn't
    // apply changes when the register is in {r9, sp, lr, pc}.
    {0xe3849055, "orr     r9, r4, #85     ; 0x55"},
    {0xe384d055, "orr     sp, r4, #85     ; 0x55"},
    {0xe384e055, "orr     lr, r4, #85     ; 0x55"},
    {0xe384f055, "orr     pc, r4, #85     ; 0x55"},
    {0xe3949055, "orrs    r9, r4, #85     ; 0x55"},
    {0xe394d055, "orrs    sp, r4, #85     ; 0x55"},
    {0xe394e055, "orrs    lr, r4, #85     ; 0x55"},
    {0xe394f055, "orrs    pc, r4, #85     ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      unchanged, NACL_ARRAY_SIZE(unchanged));
}

// Test effects of virtual dynamic_code_replacement_sentinel on the mvn
// instruction.
TEST_F(ValidatorTests, DynamicCodeReplacementSentinelMvn) {
  // Test cases where the sentinel changes for mvn.
  const AnnotatedInstruction mvn[] = {
    {0xe3e064aa, "mvn     r6, #-1442840576        ; 0xaa000000"},
    {0xe3e068aa, "mvn     r6, #11141120   ; 0xaa0000"},
    {0xe3e06caa, "mvn     r6, #43520      ; 0xaa00"},
    {0xe3e060aa, "mvn     r6, #170        ; 0xaa"},
    {0xe3e06455, "mvn     r6, #1426063360 ; 0x55000000"},
    {0xe3e06855, "mvn     r6, #5570560    ; 0x550000"},
    {0xe3e06c55, "mvn     r6, #21760      ; 0x5500"},
    {0xe3e06055, "mvn     r6, #85 ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      mvn, NACL_ARRAY_SIZE(mvn), 0xe3e06000);
  const AnnotatedInstruction mvns[] = {
    {0xe3f064aa, "mvns    r6, #-1442840576        ; 0xaa000000"},
    {0xe3f068aa, "mvns    r6, #11141120   ; 0xaa0000"},
    {0xe3f06caa, "mvns    r6, #43520      ; 0xaa00"},
    {0xe3f060aa, "mvns    r6, #170        ; 0xaa"},
    {0xe3f06455, "mvns    r6, #1426063360 ; 0x55000000"},
    {0xe3f06855, "mvns    r6, #5570560    ; 0x550000"},
    {0xe3f06c55, "mvns    r6, #21760      ; 0x5500"},
    {0xe3f06055, "mvns    r6, #85 ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_match(
      static_cast<const ValidatorTests*>(this),
      mvns, NACL_ARRAY_SIZE(mvns), 0xe3f06000);

  // Test cases where the sentinel doesn't change for orr.
  const AnnotatedInstruction unchanged[] = {
    // Note: These instructions may not validate for other reasons,
    // but we are only testing the virtual
    // dynamic_code_replacement_sentinel, and that it doesn't
    // apply changes when the register is in {r9, sp, lr, pc}.
    {0xe3e09055, "mvn     r9, #85 ; 0x55"},
    {0xe3e0d055, "mvn     sp, #85 ; 0x55"},
    {0xe3e0e055, "mvn     lr, #85 ; 0x55"},
    {0xe3e0f055, "mvn     pc, #85 ; 0x55"},
    {0xe3f09055, "mvns    r9, #85 ; 0x55"},
    {0xe3f0d055, "mvns    sp, #85 ; 0x55"},
    {0xe3f0e055, "mvns    lr, #85 ; 0x55"},
    {0xe3f0f055, "mvns    pc, #85 ; 0x55"},
  };
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      unchanged, NACL_ARRAY_SIZE(unchanged));
}

// Test other instructions for which dynamic code replacement can't be applied.
TEST_F(ValidatorTests, DynamicCodeReplacementSentinelOther) {
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      examples_of_safe_stores, NACL_ARRAY_SIZE(examples_of_safe_stores));
  test_if_dynamic_code_replacement_sentinels_unchanged(
      static_cast<const ValidatorTests*>(this),
      examples_of_safe_masks, NACL_ARRAY_SIZE(examples_of_safe_masks));
}

TEST_F(ValidatorTests, DynamicCodeReplacementLiteralPool) {
  // Start with an evil-looking value, and replace with another evil-looking
  // value. It should work because these are all data in two consecutive
  // literal pools, not actual instructions.
  vector<arm_inst>::size_type bundle_size(_validator->InstructionsPerBundle());
  vector<arm_inst>::size_type size(bundle_size * 2);
  vector<arm_inst> old_code(size, kSvc0);
  vector<arm_inst> new_code(size, kFailValidation);
  old_code[0] = new_code[0] =
      old_code[bundle_size] = new_code[bundle_size] =
      kLiteralPoolHead;
  TestDynamicCodeReplacement(
      _validator, true, old_code, new_code,
      "literal pool data change");

  // Change the new code so that its first bundle is still a literal
  // pool but its second isn't. This should now fail validation because
  // the data is now executable code.
  new_code[bundle_size] = kNop;
  TestDynamicCodeReplacement(
      _validator, false, old_code, new_code,
      "literal pool becoming non-literal pool");

  // Change the old code so that its second bundle also isn't a literal
  // pool. This should also fail, not because the code is dangerous
  // (which it is, but that's checked separately), but because the old
  // instructions can't be substituted for the new.
  old_code[bundle_size] = kNop;
  TestDynamicCodeReplacement(
      _validator, false, old_code, new_code,
      "literal pool followed by non-literal pool with invalid replacement");

  // Change the new code's second bundle to a literal pool, which it
  // wasn't in the old. This should also fail because we don't allow the
  // user to dynamically create literal pools where none existed before.
  new_code[bundle_size] = kLiteralPoolHead;
  TestDynamicCodeReplacement(
      _validator, false, old_code, new_code,
      "non-literal pool becoming literal pool");
}

struct AlwaysDominatesTestInfo {
  arm_inst inst[2];
  const char* name[2];
  bool first_inst_can_set_flags;
};
TEST_F(ValidatorTests, AlwaysDominatesTest) {
  // Test always_dominates, with all conditional combinations of:
  AlwaysDominatesTestInfo test[] = {
    { {
        // BIC (potentially setting flags) followed by a branch.
        0xe3cee2fe,  // bic[s]CC lr, lr, #-536870897     ; 0xe000000f
        0xe12fff1e,  // bxCC lr
      }, { "bic", "bx" }, true },
  };

  for (AlwaysDominatesTestInfo* t = &test[0];
       t != &test[NACL_ARRAY_SIZE(test)];
       ++t) {
    for (int s = 0; s < (t->first_inst_can_set_flags ? 2 : 1); ++s) {
      if (t->first_inst_can_set_flags) {
        t->inst[0] = SetSBit(t->inst[0], s);
      }

      Instruction::Condition cond[2];
      for (cond[0] = Instruction::EQ;
           cond[0] <= Instruction::AL;
           cond[0] = Instruction::Next(cond[0])) {
        for (cond[1] = Instruction::EQ;
             cond[1] <= Instruction::AL;
             cond[1] = Instruction::Next(cond[1])) {
          t->inst[0] = ChangeCond(t->inst[0], cond[0]);
          t->inst[1] = ChangeCond(t->inst[1], cond[1]);

          std::string name0(std::string(t->name[0]) + (s ? "s" : "") +
                            Instruction::ToString(cond[0]));
          std::string name1(std::string(t->name[1]) +
                            Instruction::ToString(cond[1]));
          ostringstream message;
          message << name0 <<
              " (0x" << std::hex << std::setfill('0') << std::setw(8) <<
              static_cast<uint32_t>(t->inst[0]) <<
              std::resetiosflags(std::ios_base::showbase) << ") "
              "with a correct mask, followed by " << name1 <<
              " (0x" << std::hex << std::setfill('0') << std::setw(8) <<
              static_cast<uint32_t>(t->inst[1]) <<
              std::resetiosflags(std::ios_base::showbase) << "): ";

          if (s) {
            validation_should_fail(t->inst, NACL_ARRAY_SIZE(t->inst),
                                   kDefaultBaseAddr,
                                   message.str() + name0 + " sets flags "
                                   "when it's only supposed to enforce SFI on "
                                   "the subsequent " + name1 + ", we "
                                   "could allow this but it makes the "
                                   "validator's code more complex and it's "
                                   "harder to reason about back-to-back "
                                   "conditional instructions with "
                                   "intervening flag setting (especially with "
                                   "ARM's partial NZCV flag setting).");
          } else if (cond[0] == Instruction::AL) {
            validation_should_pass(t->inst, NACL_ARRAY_SIZE(t->inst),
                                   kDefaultBaseAddr,
                                   message.str() + "an unconditional " +
                                   name0 + " always dominates the "
                                   "subsequent " + name1 +
                                   " instruction.");
          } else if (cond[1] == Instruction::AL) {
            EXPECT_NE(cond[0], Instruction::AL);
            validation_should_fail(t->inst, NACL_ARRAY_SIZE(t->inst),
                                   kDefaultBaseAddr,
                                   message.str() + name0 + " is "
                                   "conditional, but the subsequent " +
                                   name1 + " isn't.");
          } else if ((cond[1] == cond[0]) ||
                     // TODO(jfb) Put back mixed-condition handling.
                     //           See issue #3221.
                     /*
                   (cond[1] == Instruction::EQ && cond[0] == Instruction::LS) ||
                   (cond[1] == Instruction::CC && cond[0] == Instruction::LS) ||
                   (cond[1] == Instruction::HI && cond[0] == Instruction::NE) ||
                   (cond[1] == Instruction::HI && cond[0] == Instruction::CS) ||
                   (cond[1] == Instruction::GT && cond[0] == Instruction::NE) ||
                   (cond[1] == Instruction::GT && cond[0] == Instruction::GE) ||
                   (cond[1] == Instruction::LE && cond[0] == Instruction::EQ) ||
                   (cond[1] == Instruction::LE && cond[0] == Instruction::LS) ||
                   (cond[1] == Instruction::LE && cond[0] == Instruction::LT) */
                     false) {
            validation_should_pass(t->inst, NACL_ARRAY_SIZE(t->inst),
                                   kDefaultBaseAddr,
                                   message.str() + name1 + "'s "
                                   "condition being true necessarily implies "
                                   "that " + name0 + "'s condition was "
                                   "also true.");
          } else {
            validation_should_fail(t->inst, NACL_ARRAY_SIZE(t->inst),
                                   kDefaultBaseAddr,
                                   message.str() + name1 + "'s condition "
                                   "being true doesn't necessarily imply "
                                   "that " + name0 + "'s condition was "
                                   "also true, err on the side of caution and "
                                   "disallow.");
          }
        }
      }
    }
  }
}

TEST_F(ValidatorTests, UnmaskedSpUpdate) {
  vector<arm_inst> code(_validator->InstructionsPerBundle(), kNop);
  for (vector<arm_inst>::size_type i = 0; i < code.size(); ++i) {
    std::fill(code.begin(), code.end(), kNop);
    code[i] = 0xE1A0D000;  // MOV SP, R0
    validation_should_fail(&code[0], code.size(), kDefaultBaseAddr,
                           "unmasked SP update");
  }
}

TEST_F(ValidatorTests, MaskedSpUpdate) {
  vector<arm_inst> code(_validator->InstructionsPerBundle() * 2, kNop);
  for (vector<arm_inst>::size_type i = 0; i < code.size() - 1; ++i) {
    std::fill(code.begin(), code.end(), kNop);
    code[i] = 0xE1A0D000;      // MOV SP, R0
    code[i + 1] = 0xE3CDD2FF;  // BIC SP, SP, #-268435441 ; 0xf000000f
    if (i == _validator->InstructionsPerBundle() - 1) {
      validation_should_fail(&code[0], code.size(), kDefaultBaseAddr,
                             "masked SP update straddling a bundle boundary"
                             " (this is technically safe, but we simplify the "
                             "validator by disallowing instruction pairs that "
                             "straddle a bundle boundary)");
    } else {
      validation_should_pass(&code[0], code.size(), kDefaultBaseAddr,
                             "masked SP update");
    }
  }
}

TEST_F(ValidatorTests, AddConstToSpTest) {
  // Show that we can add a constant to the stack pointer is fine,
  // so long as we follow it with a mask instruction.
  static const arm_inst sp_inst[] = {
    0xe28dd00c,  // add sp, sp, #12
    0xe3cdd2ff,  // bic     sp, sp, #-268435441     ; 0xf000000f
  };
  validation_should_pass(sp_inst,
                         NACL_ARRAY_SIZE(sp_inst),
                         kDefaultBaseAddr,
                         "Add constant (12) to sp, then mask with bic");
}

TEST_F(ValidatorTests, BicSpFirstInstTest) {
  // Show that if we bit clear SP as the first instruction, it is ok.
  static const arm_inst bic_inst[] = {
    0xe3cdd2ff,  // bic     sp, sp, #-268435441     ; 0xf000000f
  };
  validation_should_pass(bic_inst,
                         NACL_ARRAY_SIZE(bic_inst),
                         kDefaultBaseAddr,
                         "Bit clear sp as first instruction.");
}

TEST_F(ValidatorTests, BicSpSecondInstTest) {
  // Show that if we bit clear SP as the second instruction, and
  // the first doesn't update SP, we are ok.
  static const arm_inst bic_inst[] = {
    kNop,
    0xe3cdd2ff,  // bic     sp, sp, #-268435441     ; 0xf000000f
  };
  validation_should_pass(bic_inst,
                         NACL_ARRAY_SIZE(bic_inst),
                         kDefaultBaseAddr,
                         "Bit clear sp as second instruction.");
}

TEST_F(ValidatorTests, AddConstToSpBicTestDoesFollows) {
  // Run test where we conditionally add a constant to a stack pointer,
  // followed by a mask.
  // Note: Implicitly tests always_postdominates.
  for (int cond = Instruction::EQ; cond < Instruction::AL; ++cond) {
    arm_inst inst[] = {
      0x028dd00c,  // addeq sp, sp, #12
      0xe3cdd2ff,  // bic sp, sp, #-268435441     ; 0xf000000f
    };
    inst[0] = ChangeCond(inst[0], static_cast<Instruction::Condition>(cond));
    validation_should_pass(inst,
                           NACL_ARRAY_SIZE(inst),
                           kDefaultBaseAddr,
                           "Add constant (12) to sp, then mask with bic, "
                           "tests always_postdominates succeeds");
  }
}

TEST_F(ValidatorTests, AddConstToSpBicTestDoesntFollows) {
  // Run test where we add a constant to a stack pointer, followed
  // by a conditional mask.
  // Note: Implicitly tests always_postdominates.
  for (int cond = Instruction::EQ; cond < Instruction::AL; ++cond) {
    arm_inst inst[] = {
      0xe28dd00c,  // add sp, sp, #12
      0x03cdd2ff,  // biceq sp, sp, #-268435441   ; 0xf000000f
    };
    inst[1] = ChangeCond(inst[1], static_cast<Instruction::Condition>(cond));
    validation_should_fail(inst,
                           NACL_ARRAY_SIZE(inst),
                           kDefaultBaseAddr,
                           "Add constant (12) to sp, then mask with bic, "
                           "tests always_postdominates fails");
  }
}

TEST_F(ValidatorTests, CheckVectorLoadPcRelative) {
  // Run test where we do a vector load using a pc relative address.
  // Corresponds to issue 2906.
  static const arm_inst load_inst[] = {
    0xed9f0b04,  // vldr        d0, [pc, #16]
  };
  validation_should_pass(load_inst,
                         NACL_ARRAY_SIZE(load_inst),
                         kDefaultBaseAddr,
                         "Load vector register using pc relative address");
}

TEST_F(ValidatorTests, CheckPushSpUnpredictable) {
  // Run test to verify that "Push {sp}", encoding A2 on a8-248 of ARM manual,
  // is unpredictable (i.e. unsafe).
  all_cond_values_fail(0xe52dd004,  // push {sp}
                       kDefaultBaseAddr,
                       "push {sp} (A2 a8-248) should be unpredictable");
}

TEST_F(ValidatorTests, ConditionalBreakpoints) {
  arm_inst bkpt = 0xE1200070;  // BKPT #0
  arm_inst pool_head = kLiteralPoolHead;
  for (Instruction::Condition cond = Instruction::EQ;
       cond < Instruction::AL;
       cond = Instruction::Next(cond)) {
    bkpt = ChangeCond(bkpt, cond);
    pool_head = ChangeCond(pool_head, cond);
    EXPECT_FALSE(validate(&bkpt, 1, kDefaultBaseAddr))
        << "conditional breakpoint should be unpredictable";
    EXPECT_FALSE(validate(&pool_head, 1, kDefaultBaseAddr))
        << "conditional literal pool head should be unpredictable";
  }
}

TEST_F(ValidatorTests, CheckLoadStoreWritebackUnpredictable) {
  // See issue #2816: compilers used to emit some unpredictable load/store
  // instructions with writeback, and the validator accepted them for a while.
  //
  // Theses unpredictables have writeback and either Rn=Rt or Rn=Rt2.
  //
  // The instructions are all from A5.2.8 Extra Load/Store Instructions:
  //   cccc 000P UaWa nnnn tttt xxxx 1bb1 xxxx
  // With an exclusion for data-processing and miscellaneous as noted below.
  //
  // Nibbles 0 and 2 are either zeroes/Rm (register variants) or
  // imm4H/imm4L (integer variant). Only test when they're all zero.
  arm_inst templ = 0x00000090;
  for (uint32_t op2 = 0; op2 <= 0x3; ++op2) {  // op2 is in inst(6:5).
    for (uint32_t op1 = 0; op1 <= 0x1F; ++op1) {  // op1 is in inst(24:20).
      if (!(op1 & 0x2))
        continue;  // Only undefined with writeback.
      if ((op2 == 0x0) ||
          ((op1 & 0x13) == 0x03) ||
          (((op1 & 0x13) == 0x02) && ((op2 & 0x2) == 0x0)))
        continue;  // Not in A5.2.8: actually Data-processing and miscellaneous.
      // Dual load/store encode Rt2 as Rt+1.
      bool is_dual = ((op1 & 0x01) == 0x00) && ((op2 == 0x2) || (op2 == 0x3));
      for (uint32_t n = 0; n < 15; ++n) {  // Ignore Rn=PC, it can't writeback.
        uint32_t t = n;
        arm_inst base = (templ | (op1 << 20) | (t << 12) | (op2 << 5));
        arm_inst inst = base | (n << 16);  // Rn=Rt.
        arm_inst inst_dual = base | ((n + 1) << 16);  // Rn=Rt2.
        all_cond_values_fail(inst, kDefaultBaseAddr, "extra load/store "
                             "with writeback and Rn==Rt should be "
                             "unpredictable");
        if (is_dual) {
          all_cond_values_fail(inst_dual, kDefaultBaseAddr, "extra load/store "
                               "with writeback and Rn==Rt2 should be "
                               "unpredictable");
        }
      }
    }
  }
}

TEST_F(ValidatorTests, LiteralPoolHeadIsBreakpoint) {
  EXPECT_EQ(kLiteralPoolHead & 0xFFF000F0, 0xE1200070)  // BKPT #0
      << ("the literal pool head should be a breakpoint: "
          "it needs to act as a roadblock");
}

TEST_F(ValidatorTests, Breakpoint) {
  EXPECT_EQ(nacl_arm_dec::kBreakpoint & 0xFFF000F0,
            0xE1200070)  // BKPT #0
      << ("the breakpoint instruction should be a breakpoint: "
          "it needs to trap");
}

TEST_F(ValidatorTests, HaltFill) {
  EXPECT_EQ(nacl_arm_dec::kHaltFill & 0xFFF000F0,
            0xE7F000F0)  // UDF #0
      << ("the halt fill instruction should be UDF: "
          "it needs to trap");
}

TEST_F(ValidatorTests, AbortNow) {
  EXPECT_EQ(nacl_arm_dec::kAbortNow & 0xFFF000F0,
            0xE7F000F0)  // UDF #0
      << ("the abort now instruction should be UDF: "
          "it needs to trap");
}

TEST_F(ValidatorTests, FailValidation) {
  EXPECT_EQ(kFailValidation & 0xFFF000F0, 0xE7F000F0)  // UDF #0
      << ("the fail validation instruction should be UDF: "
          "it needs to trap");
}

TEST_F(ValidatorTests, UDFAndBKPTValidateAsExpected) {
  for (uint32_t i = 0; i < 0xFFFF; ++i) {
    arm_inst bkpt_inst = 0xE1200070 | ((i & 0xFFF0) << 4) | (i & 0xF);
    arm_inst udf_inst  = 0xE7F000F0 | ((i & 0xFFF0) << 4) | (i & 0xF);
    EXPECT_EQ(validate(&bkpt_inst, 1, kDefaultBaseAddr),
              ((bkpt_inst == kLiteralPoolHead) ||
               (bkpt_inst == nacl_arm_dec::kBreakpoint)));
    EXPECT_EQ(validate(&udf_inst, 1, kDefaultBaseAddr),
              ((udf_inst == nacl_arm_dec::kHaltFill) ||
               (udf_inst == nacl_arm_dec::kAbortNow)));
    // Tautological note: kFailValidation should fail validation.
  }
}

TEST_F(ValidatorTests, LiteralPoolHeadInstruction) {
  // Make sure that literal pools are handled properly: they should be preceded
  // by a special breakpoint instruction at the start of the bundle, and can
  // then contain any bits that would otherwise look malicious.
  // Each literal pool bundle has to start with such a literal pool head.
  vector<arm_inst> literal_pool(_validator->InstructionsPerBundle(), kSvc0);
  literal_pool[0] = 0xE1200070;  // BKPT #0
  // Try out all BKPT encodings, and make sure only one of them works.
  for (uint32_t imm16 = 0; imm16 <= 0xFFFF; ++imm16) {
    literal_pool[0] = (literal_pool[0] & 0xFFF000F0) |
        (imm16 & 0xF) |
        ((imm16 & 0xFFF0) << 8);
    if (literal_pool[0] == kLiteralPoolHead) {
      validation_should_pass(&literal_pool.front(),
                             literal_pool.size(),
                             kDefaultBaseAddr,
                             "valid literal pool: "
                             "starts with special BKPT");
    } else {
      validation_should_fail(&literal_pool.front(),
                             literal_pool.size(),
                             kDefaultBaseAddr,
                             "invalid literal pool: "
                             "starts with just a regular BKPT");
    }
  }
}

TEST_F(ValidatorTests, LiteralPoolHeadPosition) {
  // Literal pools should only work when the head instruction is indeed at
  // the head.
  vector<arm_inst> literal_pool(_validator->InstructionsPerBundle());
  for (size_t pos = 0; pos < literal_pool.size(); ++pos) {
    std::fill(literal_pool.begin(), literal_pool.end(), kSvc0);
    if (pos != literal_pool.size() - 1) {
      // We do one iteration without a literal pool head at all.
      literal_pool[pos] = kLiteralPoolHead;
    }
    if (pos == 0) {
      validation_should_pass(&literal_pool.front(),
                             literal_pool.size(),
                             kDefaultBaseAddr,
                             "valid literal pool: "
                             "starts with special head instruction");
    } else {
      validation_should_fail(&literal_pool.front(),
                             literal_pool.size(),
                             kDefaultBaseAddr,
                             "invalid literal pool: "
                             "doesn't start with special  head instruction");
    }
  }
}

TEST_F(ValidatorTests, LiteralPoolBig) {
  // Literal pools should be a single bundle wide, each must be preceded by
  // a pool head.
  vector<arm_inst> literal_pools(2 * _validator->InstructionsPerBundle());
  for (size_t pos = 0; pos < literal_pools.size(); ++pos) {
    std::fill(literal_pools.begin(), literal_pools.end(), kSvc0);
    literal_pools[pos] = kLiteralPoolHead;
    validation_should_fail(&literal_pools.front(),
                           literal_pools.size(),
                           kDefaultBaseAddr,
                           "invalid literal pool: two pools, one head");
  }
}

TEST_F(ValidatorTests, LiteralPoolBranch) {
  // Branching to a literal pool should only work at the head.
  // Construct a code region with a bundle of code, then a bundle-wide
  // literal pool, then another bundle of code.
  // Add a branch from different code locations, pointing at different
  // parts of the code. Pointing in the literal pool should fail, except
  // when pointing at the head.
  // Note that we don't actually put anything malicious in the literal pool,
  // and we still shouldn't be able to jump in the middle of it.
  const size_t bundle_pos = _validator->InstructionsPerBundle();
  vector<arm_inst> code(3 * bundle_pos);
  for (size_t b_pos = 0; b_pos < code.size(); ++b_pos) {
    if ((bundle_pos <= b_pos) && (b_pos < bundle_pos * 2)) {
      // Don't try putting the branch in the middle of the literal pool.
      continue;
    }
    std::fill(code.begin(), code.end(), kNop);
    code[bundle_pos] = kLiteralPoolHead;
    for (size_t b_target = 0; b_target < code.size(); ++b_target) {
      // PC reads as current instruction address plus 8 (e.g. two instructions
      // ahead of b_pos).
      // imm24 is encoded with the bottom two bits zeroed out, which we
      // implicitly do by working with instructions instead of bytes.
      uint32_t imm24 = (b_target - b_pos - 2) & 0x00FFFFFF;
      code[b_pos] = 0xEA000000 | imm24;  // B #imm
      bool target_in_pool = (bundle_pos < b_target) &&
          (b_target < bundle_pos * 2);  // Excluding head.
      if (target_in_pool) {
        validation_should_fail(&code.front(),
                               code.size(),
                               kDefaultBaseAddr,
                               "branch inside a literal pool");
      } else {
        validation_should_pass(&code.front(),
                               code.size(),
                               kDefaultBaseAddr,
                               "branch around or at head of a literal pool");
      }
    }
  }
}

TEST_F(ValidatorTests, Preloads) {
  // Preloads leak information on some ARM CPUs and are therefore treated
  // similar to no-destination loads. They come in three flavors:
  // - PLD{W} [<Rn>, #+/-<imm12>] simply needs to mask Rn.
  // - PLD <label> doesn't need masking: its immediate is limited to 12 bits.
  // - PLD{W} [<Rn>,+/-<Rm>{, <shift>}] is disallowed.
  // The same applies for PLI, which has analogous variants.

  // PLD{W} [<Rn>, #+/-<imm12>] as well as PLI.
  // PLD{W}: 1111 0101 UR01 nnnn 1111 iiii iiii iiii
  // PLI:    1111 0100 U101 nnnn 1111 iiii iiii iiii
  for (uint32_t is_pld = 0; is_pld <= 1; ++is_pld) {
    for (uint32_t r = is_pld ? 0 : 1; r <= 1; ++r) {
      for (uint32_t u = 0; u <= 1; ++u) {
        uint32_t rn = 0x1;  // TODO(jfb) The BIC patterns only test Rn==R1.
        for (uint32_t imm12 = 0; imm12 <= 0xFFF; ++imm12) {
          arm_inst pl_inst = 0xF410F000 |
              (is_pld << 24) | (u << 23) | (r << 22) | (rn << 16) | imm12;
          validation_should_fail(&pl_inst, 1, kDefaultBaseAddr,
                                 "unmasked preloads");
          for (size_t m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks);
               ++m) {
            arm_inst program[] = {
              ChangeCond(examples_of_safe_masks[m].inst,
                         Instruction::AL),
              pl_inst,
            };
            validation_should_pass(program, NACL_ARRAY_SIZE(program),
                                   kDefaultBaseAddr,
                                   "masked preload with safe mask");
          }
          for (size_t m = 0; m < NACL_ARRAY_SIZE(examples_of_invalid_masks);
               ++m) {
            arm_inst program[] = {
              ChangeCond(examples_of_invalid_masks[m].inst,
                         Instruction::AL),
              pl_inst,
            };
            validation_should_fail(program, NACL_ARRAY_SIZE(program),
                                   kDefaultBaseAddr,
                                   "masked preload with invalid mask");
          }
        }
      }
    }
  }

  // PLD <label> as well as PLI.
  // PLD: 1111 0101 U101 1111 1111 iiii iiii iiii
  // PLI: 1111 0100 U101 1111 1111 iiii iiii iiii
  for (uint32_t is_pld = 0; is_pld <= 1; ++is_pld) {
    for (uint32_t u = 0; u <= 1; ++u) {
      for (uint32_t imm12 = 0; imm12 <= 0xFFF; ++imm12) {
        arm_inst pl_inst = 0xF45FF000 |
            (is_pld << 24) | (u << 23) | imm12;
        validation_should_pass(&pl_inst, 1, kDefaultBaseAddr,
                               "unmasked literal preloads");
      }
    }
  }

  // PLD{W} [<Rn>, +/-Rm{, shift}] as well as PLI.
  // PLD{W}: 1111 0111 UR01 nnnn 1111 iiii itt0 mmmm
  // PLI:    1111 0110 U101 nnnn 1111 iiii itt0 mmmm
  for (uint32_t is_pld = 0; is_pld <= 1; ++is_pld) {
    for (uint32_t r = is_pld ? 0 : 1; r <= 1; ++r) {
      for (uint32_t u = 0; u <= 1; ++u) {
        for (uint32_t t = 0; t <= 3; ++t) {
          for (uint32_t rm = 0; rm <= 0xF; ++rm) {
            uint32_t rn = 0x1;  // TODO(jfb) The BIC patterns only test Rn==R1.
            for (uint32_t imm5 = 0; imm5 <= 0x1F; ++imm5) {
              arm_inst pl_inst = 0xF610F000 |
                  (is_pld << 24) | (u << 23) | (r << 22) | (rn << 16) |
                  (imm5 << 7) | (t << 5) | rm;
              validation_should_fail(&pl_inst, 1, kDefaultBaseAddr,
                                     "unmasked register-register preloads");
              for (size_t m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks);
                   ++m) {
                arm_inst program[] = {
                  ChangeCond(examples_of_safe_masks[m].inst,
                             Instruction::AL),
                  pl_inst,
                };
                validation_should_fail(program, NACL_ARRAY_SIZE(program),
                                       kDefaultBaseAddr,
                                       "masked register-register preload "
                                       "with safe mask");
              }
              for (size_t m = 0; m < NACL_ARRAY_SIZE(examples_of_invalid_masks);
                   ++m) {
                arm_inst program[] = {
                  ChangeCond(examples_of_invalid_masks[m].inst,
                             Instruction::AL),
                  pl_inst,
                };
                validation_should_fail(program, NACL_ARRAY_SIZE(program),
                                       kDefaultBaseAddr,
                                       "masked register-register preload "
                                       "with invalid mask");
              }
            }
          }
        }
      }
    }
  }
}

TEST_F(ValidatorTests, LoadThreadLocalPtr) {
  // Verify that we recognize valid load thread local pointer instructions.
  static const AnnotatedInstruction loadr9_insts[] = {
    {0xe5990000,  "ldr<c> r0, [r9]"},
    {0xe5990004,  "ldr<c> r0, [r9, #4]"},
  };

  // Try each load instruction
  for (size_t i = 0; i < NACL_ARRAY_SIZE(loadr9_insts); ++i) {
    Instruction test_base(loadr9_insts[i].inst);

    // Try each possible condition.
    for (Instruction::Condition cond = Instruction::EQ;
         cond < Instruction::UNCONDITIONAL;
         cond = Instruction::Next(cond)) {
      Instruction test_cond(ChangeInstCond(test_base, cond));

      // Try each register to assign (i.e. replace r0).
      for (uint32_t r = 0; r < 16; ++r) {
        Register reg(r);
        Instruction test(test_cond);
        test.SetBits(15, 12, r);  // I.e. set Rt(15:12) to r;

        // Generate string describing instruction being tested.
        ostringstream message;
        string msg(loadr9_insts[i].about);
        const char* r0 = "r0";
        msg.replace(msg.find(r0), strlen(r0), reg.ToString());
        const char* cc = "<c>";
        msg.replace(msg.find(cc), strlen(cc), Instruction::ToString(cond));
        message << "Test '" << msg << "'";

        // Test the generated instruction.
        arm_inst insts[1] = { test.Bits() };
        if (IsValidSingleInstructionDestinationRegister(reg)) {
          validation_should_pass(insts, NACL_ARRAY_SIZE(insts),
                                 kDefaultBaseAddr, message.str());
        } else {
          validation_should_fail(insts, NACL_ARRAY_SIZE(insts),
                                 kDefaultBaseAddr, message.str());
        }
      }
    }
  }
}

TEST_F(ValidatorTests, UseR9) {
  // Verify that we report about illegal uses of thread local pointer.
  static const AnnotatedInstruction use_r9_insts[] = {
    {0xe0892009, "add r2, r9, r9"},
    { 0x03C99103, "bic r9, r9, #0xC0000000: simple in-place mask (form 1)" },
    { 0x03C91103, "bic r1, r9, #0xC0000000: mask with register move (form 1)" },
    { 0x03C19103, "bic r9, r1, #0xC0000000: mask with register move (form 1)" },
    { 0x03C994C0, "bic r9, r9, #0xC0000000: simple in-place mask (form 2)" },
    { 0x03C914C0, "bic r1, r9, #0xC0000000: mask with register move (form 2)" },
    { 0x03C394C0, "bic r9, r3, #0xC0000000: mask with register move (form 2)" },
    { 0x03C994FF, "bic r9, r9, #0xFF000000: overzealous but correct mask" },
    {0xe7df9f1f, "bfc   r9, #30, #2"},
    // TODO(karl): Add load/store examples, when implemented, like:
    //    STR r9, [rt]; MOV rd, r9; MOV sd, r9
  };

  // Try each test instruction.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(use_r9_insts); ++i) {
    Instruction test(use_r9_insts[i].inst);

    // Try each possible condition.
    for (Instruction::Condition cond = Instruction::EQ;
         cond < Instruction::UNCONDITIONAL;
         cond = Instruction::Next(cond)) {
      Instruction test_cond(ChangeInstCond(test, cond));

      ostringstream message;
      message << "Test use of r9 with cond=" << Instruction::ToString(cond)
              << ": " << use_r9_insts[i].about;
      validation_should_fail(&use_r9_insts[i].inst, 1, kDefaultBaseAddr,
                             message.str());
    }
  }
}

};  // anonymous namespace

// Test driver function.
int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
