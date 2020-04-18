/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_VALIDATOR_TESTS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_VALIDATOR_TESTS_H_

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif

/*
 * Unit tests for the ARM validator
 *
 * These tests use the google-test framework (gtest for short) to exercise the
 * ARM validator.  The tests currently fall into two rough categories:
 *  1. Simple method-level tests that exercise the validator's primitive
 *     capabilities, and
 *  2. Instruction pattern tests that run the entire validator.
 *
 * All instruction pattern tests use hand-assembled machine code fragments,
 * embedded as byte arrays.  This is somewhat ugly, but deliberate: it isolates
 * these tests from gas, which may be buggy or not installed.  It also lets us
 * hand-craft malicious bit patterns that gas may refuse to produce.
 *
 * To write a new instruction pattern, or make sense of an existing one, use the
 * ARMv7-A ARM (available online).  Instructions in this file are written as
 * 32-bit integers so the hex encoding matches the docs.
 *
 * Also see validator_tests.cc and validator_large_tests.cc
 */

#include <algorithm>
#include <climits>
#include <limits>
#include <string>
#include <sstream>
#include <vector>

#include "gtest/gtest.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability_bits.h"

#include "native_client/src/trusted/validator_arm/problem_reporter.h"
#include "native_client/src/trusted/validator_arm/validator.h"

using std::vector;
using std::string;
using std::ostringstream;

using nacl_arm_dec::kNoViolations;
using nacl_arm_dec::Instruction;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;
using nacl_arm_dec::Violation;
using nacl_arm_dec::ViolationSet;

using nacl_arm_val::SfiValidator;
using nacl_arm_val::CodeSegment;
using nacl_arm_val::ProblemReporter;
using nacl_arm_val::ProblemSink;

namespace nacl_val_arm_test {

#ifdef __BIG_ENDIAN__
#error This test will only succeed on a little-endian machine.  Sorry.
#endif

// Since ARM instructions are always little-endian, on a little-endian machine
// we can represent them as ints.  This makes things somewhat easier to read
// below.
typedef uint32_t arm_inst;

// This is where untrusted code starts.  Most tests use this.
static const uint32_t kDefaultBaseAddr = 0x20000;

/*
 * We use these parameters to initialize the validator, below.  They are
 * somewhat different from the parameters used in real NaCl systems, to test
 * degrees of validator freedom that we don't currently exercise in prod.
 */
// Number of bytes in each bundle.  Theoretically can also be 32 - not tested.
static const uint32_t kBytesPerBundle = 16;
// Limit code to 512MiB.
static const uint32_t kCodeRegionSize = 0x20000000;
// Limit data to 1GiB.
static const uint32_t kDataRegionSize = 0x40000000;
// Untrusted code must not write to the thread pointer.
static const RegisterList kAbiReadOnlyRegisters(Register::Tp());
// The stack pointer can be used for "free" loads and stores.
static const RegisterList kAbiDataAddrRegisters(nacl_arm_dec::Register::Sp());
// By default don't support TST+LDR/TST+STR, only allow it in tests that
// explicitly have them.
static const bool kAllowCondMemSfi = false;

// Simply records the arguments given to ReportProblem, below.
class ProblemRecord {
 public:
  ProblemRecord(Violation violation,
                uint32_t vaddr,
                const std::string& message)
      : violation_(violation), vaddr_(vaddr), message_(message) {}

  ProblemRecord(const ProblemRecord& r)
      : violation_(r.violation_), vaddr_(r.vaddr_), message_(r.message_) {}

  ~ProblemRecord() {}

  ProblemRecord& operator=(const ProblemRecord& r) {
    violation_ = r.violation_;
    vaddr_ = r.vaddr_;
    message_ = r.message_;
    return *this;
  }

  uint32_t vaddr() const { return vaddr_; }
  Violation violation() const { return violation_; }
  std::string message() const { return message_; }

 private:
  Violation violation_;
  uint32_t vaddr_;
  std::string message_;
};

// A ProblemSink that records all calls (implementation of the Spy pattern)
class ProblemSpy : public ProblemReporter {
 public:
  // Returns the list of found problems.
  vector<ProblemRecord> &get_problems() { return problems_; }

 protected:
  virtual void ReportProblemMessage(Violation violation,
                                    uint32_t vaddr,
                                    const char* message);

 private:
  static const vector<ProblemRecord>::size_type max_problems = 5000;
  vector<ProblemRecord> problems_;
};

// Coordinates the fixture objects used by test cases below.
class ValidatorTests : public ::testing::Test {
 public:
  // Utility method to decode an instruction.
  const nacl_arm_dec::ClassDecoder& decode(
      nacl_arm_dec::Instruction inst) const;

  // Returns the given instruction, after modifying the instruction condition
  // to the given value.
  static arm_inst ChangeCond(arm_inst inst, Instruction::Condition c) {
    return (inst & 0x0fffffff) | (static_cast<arm_inst>(c) << 28);
  }

 protected:
  ValidatorTests();

  virtual ~ValidatorTests() {
    EXPECT_EQ(_validator, (SfiValidator *) NULL);
  }

  virtual void SetUp() {
    EXPECT_EQ(_validator, (SfiValidator *) NULL);

    NaClCPUFeaturesArm cpu_features;
    NaClClearCPUFeaturesArm(&cpu_features);
    NaClSetCPUFeatureArm(&cpu_features, NaClCPUFeatureArm_CanUseTstMem,
                         kAllowCondMemSfi);

    _validator = new SfiValidator(kBytesPerBundle,
                                  kCodeRegionSize,
                                  kDataRegionSize,
                                  kAbiReadOnlyRegisters,
                                  kAbiDataAddrRegisters,
                                  &cpu_features);
  }

  virtual void  TearDown() {
    EXPECT_NE(_validator, (SfiValidator *) NULL);
    delete _validator;
    _validator = NULL;
  }

  // Utility method for validating a sequence of bytes.
  bool validate(const arm_inst *pattern,
                size_t inst_count,
                uint32_t start_addr,
                ProblemSink *sink = NULL);

  // Utility method for validating a sequence of bytes.
  ViolationSet find_violations(const arm_inst *pattern,
                               size_t inst_count,
                               uint32_t start_addr,
                               ProblemSink *sink = NULL);

  // Tests an arbitrary-size instruction fragment that is expected to pass.
  // Does not modulate or rewrite the pattern in any way.
  void validation_should_pass(const arm_inst *pattern,
                              size_t inst_count,
                              uint32_t base_addr,
                              const string &msg);

  // Tests a two-instruction pattern that's expected to pass, at each possible
  // bundle alignment. This also tries the pattern across bundle boundaries,
  // and makes sure it fails.
  void validation_should_pass2(const arm_inst *pattern,
                               size_t inst_count,
                               uint32_t base_addr,
                               const string &msg);

  // Tests a pattern that is forbidden in the SFI model.
  //
  // Note that the 'msg1' and 'msg2' parameters are merely concatentated in the
  // output.
  ViolationSet validation_should_fail(const arm_inst *pattern,
                                      size_t inst_count,
                                      uint32_t base_addr,
                                      const string &msg,
                                      ProblemSpy* spy = NULL);

  // Tests an instruction with all possible conditions (i.e. all except 1111),
  // verifying all cases are safe.
  void all_cond_values_pass(const arm_inst prototype,
                            uint32_t base_addr,
                            const string &msg);

  // Tests an instruction with all possible conditions (i.e. all except 1111),
  // verifying all cases are unsafe.
  void all_cond_values_fail(const arm_inst prototype,
                            uint32_t base_addr,
                            const string &msg);

  // Returns the given instruction, after modifying its S bit (bit 20) to
  // the given value.
  static arm_inst SetSBit(arm_inst inst, bool s) {
    return (inst & 0xffefffff) | (static_cast<arm_inst>(s) << 20);
  }

  // Returns true if the given register is not in {Tp, Sp, Pc}. That is,
  // not one of the registers requiring either: a 2-instruction sequence
  // to update (Sp and Pc); or should not be updated at all (Tp).
  bool IsValidSingleInstructionDestinationRegister(Register r) const {
    return !_is_valid_single_instruction_destination_register.Contains(r);
  }

  SfiValidator *_validator;

 private:
  RegisterList _is_valid_single_instruction_destination_register;
};

}  // namespace nacl_val_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_VALIDATOR_TESTS_H_
