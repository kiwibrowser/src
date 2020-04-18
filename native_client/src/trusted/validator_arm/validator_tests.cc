/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator_arm/validator_tests.h"

namespace nacl_val_arm_test {

void ProblemSpy::ReportProblemMessage(Violation violation,
                                      uint32_t vaddr,
                                      const char* message) {
  ProblemRecord prob(violation, vaddr, message);
  problems_.push_back(prob);
}

ValidatorTests::ValidatorTests()
    : _validator(NULL),
      _is_valid_single_instruction_destination_register() {
  _is_valid_single_instruction_destination_register.
      Add(Register::Tp()).Add(Register::Sp()).Add(Register::Pc());
}

const nacl_arm_dec::ClassDecoder& ValidatorTests::decode(
    nacl_arm_dec::Instruction inst) const {
  return _validator->decode(inst);
}

bool ValidatorTests::validate(const arm_inst *pattern,
                              size_t inst_count,
                              uint32_t start_addr,
                              ProblemSink *sink) {
  // We think in instructions; CodeSegment thinks in bytes.
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(pattern);
  CodeSegment segment(bytes, start_addr, inst_count * sizeof(arm_inst));

  vector<CodeSegment> segments;
  segments.push_back(segment);

  return _validator->validate(segments, sink);
}

ViolationSet ValidatorTests::find_violations(
    const arm_inst *pattern,
    size_t inst_count,
    uint32_t start_addr,
    ProblemSink *sink) {
  // We think in instructions; CodeSegment thinks in bytes.
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(pattern);
  CodeSegment segment(bytes, start_addr, inst_count * sizeof(arm_inst));

  vector<CodeSegment> segments;
  segments.push_back(segment);

  return _validator->find_violations(segments, sink);
}

void ValidatorTests::validation_should_pass(const arm_inst *pattern,
                                            size_t inst_count,
                                            uint32_t base_addr,
                                            const string &msg) {
  ProblemSpy spy;
  bool validation_result = validate(pattern, inst_count, base_addr, &spy);

  std::string errors;
  vector<ProblemRecord> &problems = spy.get_problems();
  for (vector<ProblemRecord>::const_iterator it = problems.begin();
       it != problems.end();
       ++it) {
    if (it != problems.begin()) {
      errors += "\n";
    }
    errors += std::string("\t") + it->message();
  }

  ASSERT_TRUE(validation_result) << msg << " should pass at " << base_addr <<
      ":\n" << errors;
}

void ValidatorTests::validation_should_pass2(const arm_inst *pattern,
                                             size_t inst_count,
                                             uint32_t base_addr,
                                             const string &msg) {
  // A couple sanity checks for correct usage.
  ASSERT_EQ(2U, inst_count)
      << "This routine only supports 2-instruction patterns.";
  ASSERT_TRUE(
      _validator->bundle_for_address(base_addr).begin_addr() == base_addr)
      << "base_addr parameter must be bundle-aligned";

  // Try error case where second instruction occurs as first instruction.
  arm_inst second_as_program[] = {
    pattern[1]
  };
  ostringstream bad_first_message;
  bad_first_message << msg << ": without first instruction";
  validation_should_fail(second_as_program,
                         NACL_ARRAY_SIZE(second_as_program),
                         base_addr,
                         bad_first_message.str());

  // Try the legitimate (non-overlapping) variations:
  uint32_t last_addr = base_addr + (kBytesPerBundle - 4);
  for (uint32_t addr = base_addr; addr < last_addr; addr += 4) {
    validation_should_pass(pattern, inst_count, addr, msg);
  }

  // Make sure it fails over bundle boundaries.
  ProblemSpy spy;
  ViolationSet violations = find_violations(
      pattern, inst_count, last_addr, &spy);

  EXPECT_NE(violations, kNoViolations)
      << msg << " should fail at overlapping address " << last_addr;
  EXPECT_TRUE(nacl_arm_dec::ContainsCrossesBundleViolation(violations))
      << msg << " should contain crosses bundle violation";

  vector<ProblemRecord> &problems = spy.get_problems();
  ASSERT_EQ(1U, problems.size())
      << msg << " should have 1 problem at overlapping address " << last_addr;

  ProblemRecord first = problems[0];

  if (first.violation() !=
      nacl_arm_dec::DATA_REGISTER_UPDATE_CROSSES_BUNDLE_VIOLATION) {
    last_addr += 4;
  }
  EXPECT_EQ(last_addr, first.vaddr())
      << "Problem in valid but mis-aligned pseudo-instruction ("
      << msg
      << ") must be reported at end of bundle";
  EXPECT_FALSE(nacl_arm_dec::IsSafetyViolation(first.violation()))
      << "Just crossing a bundle should not make a safe instruction unsafe: "
      << msg;

  // Be sure that we get one of the crosses bundle error messages.
  EXPECT_TRUE(nacl_arm_dec::IsCrossesBundleViolation(first.violation()));
}

ViolationSet ValidatorTests::validation_should_fail(
    const arm_inst *pattern,
    size_t inst_count,
    uint32_t base_addr,
    const string &msg,
    ProblemSpy* spy) {
  // TODO(cbiffle): test at various overlapping and non-overlapping addresses,
  // like above.  Not that this is a spectacularly likely failure case, but
  // it's worth exercising.
  ViolationSet violations =
      find_violations(pattern, inst_count, base_addr, spy);
  EXPECT_NE(violations, kNoViolations) << "Expected to fail: " << msg;

  if (spy != NULL) {
    // There should be at least one problem.
    vector<ProblemRecord> &problems = spy->get_problems();
    EXPECT_LT(static_cast<size_t>(0), problems.size());

    // Violations found in problems should match violations returned by
    // find_violations.
    ViolationSet problem_violations = kNoViolations;
    for (vector<ProblemRecord>::iterator iter = problems.begin();
         iter != problems.end();
         ++iter) {
      problem_violations = nacl_arm_dec::ViolationUnion(
          problem_violations,
          nacl_arm_dec::ViolationBit(iter->violation()));
    }
    EXPECT_EQ(violations, problem_violations)
        << "Violation differences: " << msg;
  }

  // The rest of the checking is done in the caller.
  return violations;
}

void ValidatorTests::all_cond_values_pass(const arm_inst prototype,
                                          uint32_t base_addr,
                                          const string &msg) {
  arm_inst test_inst = prototype;
  for (Instruction::Condition cond = Instruction::EQ;
       cond < Instruction::UNCONDITIONAL;
       cond = Instruction::Next(cond)) {
    test_inst = ChangeCond(test_inst, cond);
    EXPECT_TRUE(validate(&test_inst, 1, base_addr))
        << "Fails on cond " << Instruction::ToString(cond) << ": " << msg;
  }
}

void ValidatorTests::all_cond_values_fail(const arm_inst prototype,
                                          uint32_t base_addr,
                                          const string &msg) {
  arm_inst test_inst = prototype;
  for (Instruction::Condition cond = Instruction::EQ;
       cond < Instruction::UNCONDITIONAL;
       cond = Instruction::Next(cond)) {
    test_inst = ChangeCond(test_inst, cond);
    EXPECT_FALSE(validate(&test_inst, 1, base_addr))
      << std::hex << test_inst
        << ": Passes on cond " << Instruction::ToString(cond) << ": " << msg;
  }
}

}  // namespace nacl_val_arm_test
