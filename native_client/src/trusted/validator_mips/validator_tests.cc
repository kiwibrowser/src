/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Unit tests for the MIPS validator
 *
 * These tests use the google-test framework (gtest for short) to exercise the
 * MIPS validator.  The tests currently fall into two rough categories:
 *  1. Simple method-level tests that exercise the validator's primitive
 *     capabilities, and
 *  2. Instruction pattern tests that run the entire validator.
 *
 * All instruction pattern tests use hand-assembled machine code fragments,
 * embedded as byte arrays.  This is somewhat ugly, but deliberate: it isolates
 * these tests from gas, which may be buggy or not installed.  It also lets us
 * hand-craft malicious bit patterns that gas may refuse to produce.
 *
 * To write a new instruction pattern, or make sense of an existing one, use
 * MIPS32 Instruction Set Reference (available online). Instructions in this
 * file are written as 32-bit integers so the hex encoding matches the docs.
 */

#include <vector>
#include <string>
#include <sstream>

#include "gtest/gtest.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/trusted/validator_mips/validator.h"

using std::vector;
using std::string;
using std::ostringstream;

using nacl_mips_dec::Register;
using nacl_mips_dec::RegisterList;
using nacl_mips_dec::Instruction;

using nacl_mips_val::SfiValidator;
using nacl_mips_val::ProblemSink;
using nacl_mips_val::CodeSegment;
using nacl_mips_dec::kInstrSize;
using nacl_mips_dec::kNop;

namespace {

typedef uint32_t mips_inst;

/*
 * We use these parameters to initialize the validator, below.  They are
 * somewhat different from the parameters used in real NaCl systems, to test
 * degrees of validator freedom that we don't currently exercise in prod.
 */
// Number of bytes in each bundle.
static const uint32_t kBytesPerBundle = 16;
// Limit code to 256MiB.
static const uint32_t kCodeRegionSize = 0x10000000;
// Limit data to 1GiB.
static const uint32_t kDataRegionSize = 0x40000000;


/*
 * Support code
 */

// Simply records the arguments given to report_problem, below.
struct ProblemRecord {
  uint32_t vaddr;
  nacl_mips_dec::SafetyLevel safety;
  nacl::string problem_code;
  uint32_t ref_vaddr;
};

// A ProblemSink that records all calls (implementation of the Spy pattern)
class ProblemSpy : public ProblemSink {
 public:
  virtual void ReportProblem(uint32_t vaddr, nacl_mips_dec::SafetyLevel safety,
      const nacl::string &problem_code, uint32_t ref_vaddr = 0) {
    _problems.push_back(
        (ProblemRecord) { vaddr, safety, problem_code, ref_vaddr });
  }

  /*
   * We want *all* the errors that the validator produces. Note that this means
   * we're not testing the should_continue functionality. This is probably
   * okay.
   */
  virtual bool ShouldContinue() { return true; }

  vector<ProblemRecord> &problems() { return _problems; }

 private:
  vector<ProblemRecord> _problems;
};

/*
 * Coordinates the fixture objects used by test cases below. This is
 * forward-declared to the greatest extent possible so we can get on to the
 * important test stuff below.
 */
class ValidatorTests : public ::testing::Test {
 protected:
  ValidatorTests();

  // Utility method for validating a sequence of bytes.
  bool Validate(const mips_inst *pattern,
                size_t inst_count,
                uint32_t start_addr,
                ProblemSink *sink);

  /*
   * Tests a pattern that's expected to pass.
   */
  void ValidationShouldPass(const mips_inst *pattern,
                            size_t inst_count,
                            uint32_t base_addr,
                            const string &msg);

  /*
   * Tests a pattern that is forbidden in the SFI model.
   *
   * Note that the 'msg1' and 'msg2' parameters are merely concatentated in the
   * output.
   */
  vector<ProblemRecord> ValidationShouldFail(const mips_inst *pattern,
                                             size_t inst_count,
                                             uint32_t base_addr,
                                             const string &msg);

  SfiValidator _validator;
};


/*
 * Primitive tests checking various constructor properties.  Any of these
 * failing would be a very bad sign indeed.
 */

TEST_F(ValidatorTests, RecognizesDataAddressRegisters) {
  /*
   * Note that the logic below needs to be kept in sync with the implementation
   * of RegisterList::DataAddrRegs().
   *
   * This test is pretty trivial -- we can exercise the data_address_register
   * functionality more deeply with pattern tests below.
   */
  for (int i = 0; i < 31; i++) {
    Register reg(i);
    if (reg.Equals(Register::Sp()) || reg.Equals(Register::Tls())) {
      EXPECT_TRUE(_validator.IsDataAddressRegister(reg))
          << "Stack pointer and TLS register must be data address registers.";
    } else {
      EXPECT_FALSE(_validator.IsDataAddressRegister(reg))
          << "Only the stack pointer and TLS register are data "
             "address registers.";
    }
  }
}

/*
 * Code validation tests
 */

// This is where untrusted code starts.  Most tests use this.
static const uint32_t kDefaultBaseAddr = 0x20000;

/*
 * Here are some examples of safe stores permitted in a Native Client
 * program for MIPS32.
 */

struct AnnotatedInstruction {
  mips_inst inst;
  const char *about;
};

static const AnnotatedInstruction examples_of_safe_stores[] = {
  { 0xa1490200, "sb t1, 1024(t2) : store byte" },
  { 0xad490200, "sw t1, 1024(t2) : store word" },
  { 0xa5490200, "sh t1, 1024(t2) : store halfword" },
  { 0xa9490200, "swl t1, 1024(t2) : store word left" },
  { 0xb9490200, "swr t1, 1024(t2) : store word right" },
};

static const AnnotatedInstruction examples_of_safe_masks[] = {
  { (10 << 21 | 15 << 16 | 10 << 11 | 36),
    "and t2,t2,t7 : simple store masking" },
};

TEST_F(ValidatorTests, SafeMaskedStores) {
  /*
   * Produces examples of masked stores using the safe store table (above)
   * and the list of possible masking instructions (above).
   */

  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_masks); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stores);
         s++) {
      ostringstream message;
      message << examples_of_safe_masks[m].about
              << ", "
              << examples_of_safe_stores[s].about;
      mips_inst program[] = {
        examples_of_safe_masks[m].inst,
        examples_of_safe_stores[s].inst,
      };
      ValidationShouldPass(program,
                           2,
                           kDefaultBaseAddr,
                           message.str());
    }
  }
}

TEST_F(ValidatorTests, IncorrectStores) {
  /*
   * Produces incorrect examples of masked stores using the safe store table
   * (above) and the list of possible masking instructions (above).
   * By switching order of instructions (first store, then mask instruction)
   * we form incorrect pseudo-instruction.
   */

  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_stores); m++) {
     for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_masks); s++) {
       ostringstream bad_message;
       bad_message << examples_of_safe_stores[m].about
                   << ", "
                   << examples_of_safe_masks[s].about;
       mips_inst bad_program[] = {
           examples_of_safe_stores[m].inst,
           examples_of_safe_masks[s].inst,
       };

       ValidationShouldFail(bad_program,
                            2,
                            kDefaultBaseAddr,
                            bad_message.str());
    }
  }
}

static const AnnotatedInstruction examples_of_safe_jump_masks[] = {
  { (10 << 21 | 14 << 16 | 10 << 11 | 36),
    "and t2,t2,t6 : simple jump masking" },
};

static const AnnotatedInstruction examples_of_safe_jumps[] = {
  { ((10<<21| 31 << 11 |9) ), "simple jump jalr t2" },
  { (10<<21|8),               "simple jump jr t2" },
};

TEST_F(ValidatorTests, SafeMaskedJumps) {
  /*
   * Produces examples of masked jumps using the safe jump table
   * (above) and the list of possible masking instructions (above).
   */
  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_jump_masks); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_jumps); s++) {
      ostringstream message;
      message << examples_of_safe_jump_masks[m].about
              << ", "
              << examples_of_safe_jumps[s].about;
      mips_inst program[] = {
        kNop,
        examples_of_safe_jump_masks[m].inst,
        examples_of_safe_jumps[s].inst,
      };
      ValidationShouldPass(program,
                           3,
                           kDefaultBaseAddr,
                           message.str());
    }
  }
}

TEST_F(ValidatorTests, IncorrectJumps) {
  /*
   * Produces examples of incorrect jumps using the safe jump table
   * (above) and the list of possible masking instructions (above).
   * By switching order of instructions (first jump, then mask instruction)
   * we form incorrect pseudo-instruction.
   */
  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_jumps); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_jump_masks);
         s++) {
      ostringstream bad_message;
      bad_message << examples_of_safe_jumps[m].about
                  << ", "
                  << examples_of_safe_jump_masks[s].about;

      mips_inst bad_program[] = {
        examples_of_safe_jumps[m].inst,
        examples_of_safe_jump_masks[s].inst,
      };

      ValidationShouldFail(bad_program,
                           3,
                           kDefaultBaseAddr,
                           bad_message.str());
    }
  }
}

static const AnnotatedInstruction examples_of_safe_stack_masks[] = {
  { (29 << 21 | 15 << 16 | 29 << 11 | 36),
    "and sp,sp,t7 : simple stack masking" },
};


static const AnnotatedInstruction examples_of_safe_stack_ops[] = {
  { (29<<21|8<<16|29<<11|32), "add  sp,sp,t0 : stack addition" },
  { (29<<21|8<<16|29<<11|34), "sub  sp,sp,t0 : stack substraction" },
};

TEST_F(ValidatorTests, SafeMaskedStack) {
  /*
   * Produces examples of safe pseudo-ops on stack using the safe stack op table
   * and the list of possible masking instructions (above).
   */
  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_stack_ops); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stack_masks);
         s++) {
      ostringstream message;
      message << examples_of_safe_stack_ops[m].about
              << ", "
              << examples_of_safe_stack_masks[s].about;
      mips_inst program[] = {
         examples_of_safe_stack_ops[m].inst,
         examples_of_safe_stack_masks[s].inst,
      };
      ValidationShouldPass(program,
                           2,
                           kDefaultBaseAddr,
                           message.str());
    }
  }
}

TEST_F(ValidatorTests, IncorrectStackOps) {
  /*
   * Produces examples of incorrect pseudo-ops on stack using the safe stack op
   * table and the list of possible masking instructions (above).
   * With switching order of instructions (first mask instruction, then stack
   * operation) we form incorrect pseudo-instruction.
   */
  for (unsigned m = 0; m < NACL_ARRAY_SIZE(examples_of_safe_stack_masks); m++) {
    for (unsigned s = 0; s < NACL_ARRAY_SIZE(examples_of_safe_stack_ops); s++) {
      ostringstream bad_message;
      bad_message << examples_of_safe_stack_masks[m].about
                  << ", "
                  << examples_of_safe_stack_ops[s].about;
      mips_inst bad_program[] = {
        examples_of_safe_stack_masks[m].inst,
        examples_of_safe_stack_ops[s].inst,
        kNop
      };

      ValidationShouldFail(bad_program,
                           3,
                           kDefaultBaseAddr,
                           bad_message.str());
    }
  }
}

TEST_F(ValidatorTests, NopBundle) {
  vector<mips_inst> code(_validator.bytes_per_bundle() / kInstrSize, kNop);
  ValidationShouldPass(&code[0], code.size(), kDefaultBaseAddr,
                       "NOP bundle");
}

TEST_F(ValidatorTests, UnmaskedSpUpdate) {
  vector<mips_inst> code(_validator.bytes_per_bundle() / kInstrSize, kNop);
  for (vector<mips_inst>::size_type i = 0; i < code.size(); ++i) {
    std::fill(code.begin(), code.end(), kNop);
    code[i] = 0x8fbd0000;  // lw $sp, 0($sp)
    ValidationShouldFail(&code[0], code.size(), kDefaultBaseAddr,
                         "unmasked SP update");
  }
}

TEST_F(ValidatorTests, MaskedSpUpdate) {
  vector<mips_inst> code((_validator.bytes_per_bundle() / kInstrSize) * 2,
                         kNop);
  for (vector<mips_inst>::size_type i = 0; i < code.size() - 1; ++i) {
    std::fill(code.begin(), code.end(), kNop);
    code[i] = examples_of_safe_stack_ops[0].inst;
    code[i + 1] = examples_of_safe_stack_masks[0].inst;
    ValidationShouldPass(&code[0], code.size(), kDefaultBaseAddr,
                         "masked SP update");
  }
}

/*
 * Implementation of the ValidatorTests utility methods.  These are documented
 * toward the top of this file.
 */
ValidatorTests::ValidatorTests()
  : _validator(kBytesPerBundle,
               kCodeRegionSize,
               kDataRegionSize,
               RegisterList::ReservedRegs(),
               RegisterList::DataAddrRegs()) {}

bool ValidatorTests::Validate(const mips_inst *pattern,
                              size_t inst_count,
                              uint32_t start_addr,
                              ProblemSink *sink) {
  // We think in instructions; CodeSegment thinks in bytes.
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(pattern);
  CodeSegment segment(bytes, start_addr, inst_count * sizeof(mips_inst));

  vector<CodeSegment> segments;
  segments.push_back(segment);

  return _validator.Validate(segments, sink);
}

void ValidatorTests::ValidationShouldPass(const mips_inst *pattern,
                                          size_t inst_count,
                                          uint32_t base_addr,
                                          const string &msg) {
  ASSERT_TRUE(
      _validator.BundleForAddress(base_addr).BeginAddr() == base_addr)
      << "base_addr parameter must be bundle-aligned";

  ProblemSpy spy;

  bool validation_result = Validate(pattern, inst_count, base_addr, &spy);
  ASSERT_TRUE(validation_result) << msg << " should pass at " << base_addr;
  vector<ProblemRecord> &problems = spy.problems();
  EXPECT_EQ(0U, problems.size()) << msg
      << " should have no problems when located at " << base_addr;
}

vector<ProblemRecord> ValidatorTests::ValidationShouldFail(
    const mips_inst *pattern,
    size_t inst_count,
    uint32_t base_addr,
    const string &msg) {

  ProblemSpy spy;
  bool validation_result = Validate(pattern, inst_count, base_addr, &spy);
  EXPECT_FALSE(validation_result) << "Expected to fail: " << msg;

  vector<ProblemRecord> problems = spy.problems();
  // Would use ASSERT here, but cannot ASSERT in non-void functions :-(
  EXPECT_NE(0U, problems.size())
      << "Must report validation problems: " << msg;

  // The rest of the checking is done in the caller.
  return problems;
}

};  // anonymous namespace

// Test driver function.
int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
