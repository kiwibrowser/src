/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

#include "native_client/src/trusted/validator_arm/actual_vs_baseline.h"

#include "gtest/gtest.h"

namespace nacl_arm_test {

ActualVsBaselineTester::ActualVsBaselineTester(
    const NamedClassDecoder& actual,
    DecoderTester& baseline_tester)
    : Arm32DecoderTester(baseline_tester.ExpectedDecoder()),
      actual_(actual),
      baseline_(baseline_tester.ExpectedDecoder()),
      baseline_tester_(baseline_tester),
      actual_decoder_(actual_.named_decoder()),
      baseline_decoder_(baseline_.named_decoder()) {}

bool ActualVsBaselineTester::DoApplySanityChecks() {
  return baseline_tester_.ApplySanityChecks(
      inst_, baseline_tester_.GetInstDecoder());
}

void ActualVsBaselineTester::ProcessMatch() {
  baseline_tester_.InjectInstruction(inst_);
  const NamedClassDecoder& decoder = baseline_tester_.GetInstDecoder();
  if (!baseline_tester_.PassesParsePreconditions(inst_, decoder)) {
    // Parse precondition implies that this pattern is NOT supposed to
    // be handled by the baseline tester (because another decoder
    // handles it). Hence, don't do any further processing.
    return;
  }
  if (nacl_arm_dec::MAY_BE_SAFE == decoder.safety(inst_)) {
    // Run sanity baseline checks, to see that the baseline is
    // correct.
    if (!DoApplySanityChecks()) {
      // The sanity checks found a serious issue and already reported
      // it.  don't bother to report additional problems.
      return;
    }
    // Check virtuals. Start with check if safe. If unsafe, no further
    // checking need be applied, since the validator will stop
    // processing such instructions after the safe test.
    if (!MayBeSafe()) return;
  } else {
    // Not safe in baseline, only compare safety, testing if not safe
    // in actual as well.
    MayBeSafe();
    return;
  }

  // If reached, the instruction passed the initial safety check, and
  // will use the other virtual methods of the class decoders to
  // determine if the instruction is safe. Also verifies that we get
  // consistent behaviour between the actual and basline class
  // decoders.
  CheckDefs();
  CheckUses();
  CheckBaseAddressRegisterWritebackSmallImmediate();
  CheckBaseAddressRegister();
  CheckIsLiteralLoad();
  CheckBranchTargetRegister();
  CheckIsRelativeBranch();
  CheckBranchTargetOffset();
  CheckIsLiteralPoolHead();
  CheckClearsBits();
  CheckSetsZIfBitsClear();
}

bool ActualVsBaselineTester::ApplySanityChecks(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder) {
  UNREFERENCED_PARAMETER(inst);
  UNREFERENCED_PARAMETER(decoder);
  EXPECT_TRUE(false) << "Sanity Checks shouldn't be applied!";
  return false;
}

const NamedClassDecoder& ActualVsBaselineTester::ExpectedDecoder() const {
  return baseline_;
}

bool ActualVsBaselineTester::MayBeSafe() {
  // Note: We don't actually check if safety is identical. All we worry
  // about is that the validator (in sel_ldr) accepts/rejects the same
  // way in terms of safety. We don't worry if the reasons for safety
  // failing is the same.
  if (actual_decoder_.safety(inst_) == nacl_arm_dec::MAY_BE_SAFE) {
    NC_EXPECT_EQ_PRECOND(nacl_arm_dec::MAY_BE_SAFE,
                         baseline_decoder_.safety(inst_));
    return true;
  } else {
    NC_EXPECT_NE_PRECOND(nacl_arm_dec::MAY_BE_SAFE,
                         baseline_decoder_.safety(inst_));
    return false;
  }
}

void ActualVsBaselineTester::CheckDefs() {
  nacl_arm_dec::RegisterList actual_defs(actual_decoder_.defs(inst_));
  nacl_arm_dec::RegisterList baseline_defs(baseline_decoder_.defs(inst_));
  EXPECT_TRUE(baseline_defs.Equals(actual_defs));
}

void ActualVsBaselineTester::CheckUses() {
  nacl_arm_dec::RegisterList actual_uses(actual_decoder_.uses(inst_));
  nacl_arm_dec::RegisterList baseline_uses(baseline_decoder_.uses(inst_));
  EXPECT_TRUE(baseline_uses.Equals(actual_uses));
}

void ActualVsBaselineTester::CheckBaseAddressRegisterWritebackSmallImmediate() {
  EXPECT_EQ(
      baseline_decoder_.base_address_register_writeback_small_immediate(inst_),
      actual_decoder_.base_address_register_writeback_small_immediate(inst_));
}

void ActualVsBaselineTester::CheckBaseAddressRegister() {
  EXPECT_TRUE(
      baseline_decoder_.base_address_register(inst_).Equals(
          actual_decoder_.base_address_register(inst_)));
}

void ActualVsBaselineTester::CheckIsLiteralLoad() {
  EXPECT_EQ(baseline_decoder_.is_literal_load(inst_),
            actual_decoder_.is_literal_load(inst_));
}

void ActualVsBaselineTester::CheckBranchTargetRegister() {
  EXPECT_TRUE(
      baseline_decoder_.branch_target_register(inst_).Equals(
          actual_decoder_.branch_target_register(inst_)));
}

void ActualVsBaselineTester::CheckIsRelativeBranch() {
  EXPECT_EQ(baseline_decoder_.is_relative_branch(inst_),
            actual_decoder_.is_relative_branch(inst_));
}

void ActualVsBaselineTester::CheckBranchTargetOffset() {
  EXPECT_EQ(baseline_decoder_.branch_target_offset(inst_),
            actual_decoder_.branch_target_offset(inst_));
}

void ActualVsBaselineTester::CheckIsLiteralPoolHead() {
  EXPECT_EQ(baseline_decoder_.is_literal_pool_head(inst_),
            actual_decoder_.is_literal_pool_head(inst_));
}

// Mask to use if code bundle size is 16.
static const uint32_t code_mask = 15;

// Mask to use if data block is 16 bytes.
static const uint32_t data16_mask = 0xFFFF;

// Mask to use if data block is 24 bytes.
static const uint32_t data24_mask = 0xFFFFFF;

void ActualVsBaselineTester::CheckClearsBits() {
  // Note: We don't actually test all combinations. We just try a few.

  // Assuming code bundle size is 16, see if we are the same.
  EXPECT_EQ(baseline_decoder_.clears_bits(inst_, code_mask),
            actual_decoder_.clears_bits(inst_, code_mask));

  // Assume data division size is 16 bytes.
  EXPECT_EQ(baseline_decoder_.clears_bits(inst_, data16_mask),
            actual_decoder_.clears_bits(inst_, data16_mask));

  // Assume data division size is 24 bytes.
  EXPECT_EQ(baseline_decoder_.clears_bits(inst_, data24_mask),
            actual_decoder_.clears_bits(inst_, data24_mask));
}

void ActualVsBaselineTester::CheckSetsZIfBitsClear() {
  // Note: We don't actually test all combinations of masks. We just
  // try a few.
  for (uint32_t i = 0; i < 15; ++i) {
    nacl_arm_dec::Register r(i);
    EXPECT_EQ(baseline_decoder_.sets_Z_if_bits_clear(inst_, r, data24_mask),
              actual_decoder_.sets_Z_if_bits_clear(inst_, r, data24_mask));
  }
}

}  // namespace nacl_arm_test
