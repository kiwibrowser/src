/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_ACTUAL_VS_BASELINE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_ACTUAL_VS_BASELINE_H_

#include "native_client/src/trusted/validator_arm/decoder_tester.h"

namespace nacl_arm_test {

// The baseline class decoders implement class decoders that match the
// data layouts defined in "ARM Architecture Reference Manual ARM*v7-A
// and ARM*v7-R edition, Errata markup". The actual class decoders are
// the ones we are using in the validator. In general, an actual class
// decoder may be associated with many baseline class decoders (for
// example, we would like to use one "No-op" class decoder for all
// instructions that we don't care about in validation).
//
// This file defines a tester that compares baseline to actual. It
// does this by taking a baseline tester, and an actual class decoder
// It assumes that the actual class decoder is acceptable (at least
// for the baseline that is being tested) if the virtual functions
// of the two class decoders (the actual class decoder and the baseline
// class decoder associated with the tester) behave the same.
class ActualVsBaselineTester : public Arm32DecoderTester {
 public:
  ActualVsBaselineTester(const NamedClassDecoder& actual,
                         DecoderTester& baseline_tester);

  // Runs baseline tester on inputs, then checks that
  // virtuals of actual and baseline match.
  virtual void ProcessMatch();
  virtual const NamedClassDecoder& ExpectedDecoder() const;
  virtual bool ApplySanityChecks(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);

 protected:
  // Note: we quit early if the safety values are not MAY_BE_SAFE
  bool MayBeSafe();

  // Applies Sanity checks, as defined by the baseline tester.
  // Returns true if the sanity checks pass.
  virtual bool DoApplySanityChecks();

  void CheckDefs();
  void CheckUses();
  void CheckBaseAddressRegisterWritebackSmallImmediate();
  void CheckBaseAddressRegister();
  void CheckIsLiteralLoad();
  void CheckBranchTargetRegister();
  void CheckIsRelativeBranch();
  void CheckBranchTargetOffset();
  void CheckIsLiteralPoolHead();
  void CheckClearsBits();
  void CheckSetsZIfBitsClear();

  // Holds the arguments to use in comparisons.
  const NamedClassDecoder& actual_;
  const NamedClassDecoder& baseline_;
  DecoderTester& baseline_tester_;

  // These are added to cut down virtual redirection and
  // speed up tests by about 33%.
  const nacl_arm_dec::ClassDecoder& actual_decoder_;
  const nacl_arm_dec::ClassDecoder& baseline_decoder_;

 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(ActualVsBaselineTester);
};

}  // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_ACTUAL_VS_BASELINE_H_
