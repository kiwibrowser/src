// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/ninja_action_target_writer.h"
#include "tools/gn/ninja_target_writer.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

namespace {

class TestingNinjaTargetWriter : public NinjaTargetWriter {
 public:
  TestingNinjaTargetWriter(const Target* target,
                           const Toolchain* toolchain,
                           std::ostream& out)
      : NinjaTargetWriter(target, out) {
  }

  void Run() override {}

  // Make this public so the test can call it.
  std::vector<OutputFile> WriteInputDepsStampAndGetDep(
      const std::vector<const Target*>& extra_hard_deps,
      size_t num_stamp_uses) {
    return NinjaTargetWriter::WriteInputDepsStampAndGetDep(extra_hard_deps,
                                                           num_stamp_uses);
  }
};

}  // namespace

TEST(NinjaTargetWriter, WriteInputDepsStampAndGetDep) {
  TestWithScope setup;
  Err err;

  // Make a base target that's a hard dep (action).
  Target base_target(setup.settings(), Label(SourceDir("//foo/"), "base"));
  base_target.set_output_type(Target::ACTION);
  base_target.visibility().SetPublic();
  base_target.SetToolchain(setup.toolchain());
  base_target.action_values().set_script(SourceFile("//foo/script.py"));

  // Dependent target that also includes a source prerequisite (should get
  // included) and a source (should not be included).
  Target target(setup.settings(), Label(SourceDir("//foo/"), "target"));
  target.set_output_type(Target::EXECUTABLE);
  target.visibility().SetPublic();
  target.SetToolchain(setup.toolchain());
  target.config_values().inputs().push_back(SourceFile("//foo/input.txt"));
  target.sources().push_back(SourceFile("//foo/source.txt"));
  target.public_deps().push_back(LabelTargetPair(&base_target));

  // Dependent action to test that action sources will be treated the same as
  // inputs.
  Target action(setup.settings(), Label(SourceDir("//foo/"), "action"));
  action.set_output_type(Target::ACTION);
  action.visibility().SetPublic();
  action.SetToolchain(setup.toolchain());
  action.action_values().set_script(SourceFile("//foo/script.py"));
  action.sources().push_back(SourceFile("//foo/action_source.txt"));
  action.public_deps().push_back(LabelTargetPair(&target));

  ASSERT_TRUE(base_target.OnResolved(&err));
  ASSERT_TRUE(target.OnResolved(&err));
  ASSERT_TRUE(action.OnResolved(&err));

  // Input deps for the base (should be only the script itself).
  {
    std::ostringstream stream;
    TestingNinjaTargetWriter writer(&base_target, setup.toolchain(), stream);
    std::vector<OutputFile> dep =
        writer.WriteInputDepsStampAndGetDep(std::vector<const Target*>(), 10u);

    // Since there is only one dependency, it should just be returned and
    // nothing written to the stream.
    ASSERT_EQ(1u, dep.size());
    EXPECT_EQ("../../foo/script.py", dep[0].value());
    EXPECT_EQ("", stream.str());
  }

  // Input deps for the target (should depend on the base).
  {
    std::ostringstream stream;
    TestingNinjaTargetWriter writer(&target, setup.toolchain(), stream);
    std::vector<OutputFile> dep =
        writer.WriteInputDepsStampAndGetDep(std::vector<const Target*>(), 10u);

    // Since there is only one dependency, a stamp file will be returned
    // directly without writing any additional rules.
    ASSERT_EQ(1u, dep.size());
    EXPECT_EQ("obj/foo/base.stamp", dep[0].value());
  }

  {
    std::ostringstream stream;
    NinjaActionTargetWriter writer(&action, stream);
    writer.Run();
    EXPECT_EQ(
        "rule __foo_action___rule\n"
        "  command =  ../../foo/script.py\n"
        "  description = ACTION //foo:action()\n"
        "  restat = 1\n"
        "\n"
        "build: __foo_action___rule | ../../foo/script.py"
        " ../../foo/action_source.txt ./target\n"
        "\n"
        "build obj/foo/action.stamp: stamp\n",
        stream.str());
  }

  // Input deps for action which should depend on the base since its a hard dep
  // that is a (indirect) dependency, as well as the the action source.
  {
    std::ostringstream stream;
    TestingNinjaTargetWriter writer(&action, setup.toolchain(), stream);
    std::vector<OutputFile> dep =
        writer.WriteInputDepsStampAndGetDep(std::vector<const Target*>(), 10u);

    ASSERT_EQ(1u, dep.size());
    EXPECT_EQ("obj/foo/action.inputdeps.stamp", dep[0].value());
    EXPECT_EQ(
        "build obj/foo/action.inputdeps.stamp: stamp ../../foo/script.py "
        "../../foo/action_source.txt ./target\n",
        stream.str());
  }
}

// Tests WriteInputDepsStampAndGetDep when toolchain deps are present.
TEST(NinjaTargetWriter, WriteInputDepsStampAndGetDepWithToolchainDeps) {
  TestWithScope setup;
  Err err;

  // Toolchain dependency. Here we make a target in the same toolchain for
  // simplicity, but in real life (using the Builder) this would be rejected
  // because it would be a circular dependency (the target depends on its
  // toolchain, and the toolchain depends on this target).
  Target toolchain_dep_target(setup.settings(),
                              Label(SourceDir("//foo/"), "setup"));
  toolchain_dep_target.set_output_type(Target::ACTION);
  toolchain_dep_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(toolchain_dep_target.OnResolved(&err));
  setup.toolchain()->deps().push_back(LabelTargetPair(&toolchain_dep_target));

  // Make a binary target
  Target target(setup.settings(), Label(SourceDir("//foo/"), "target"));
  target.set_output_type(Target::EXECUTABLE);
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream stream;
  TestingNinjaTargetWriter writer(&target, setup.toolchain(), stream);
  std::vector<OutputFile> dep =
      writer.WriteInputDepsStampAndGetDep(std::vector<const Target*>(), 10u);

  // Since there is more than one dependency, a stamp file will be returned
  // and the rule for the stamp file will be written to the stream.
  ASSERT_EQ(1u, dep.size());
  EXPECT_EQ("obj/foo/setup.stamp", dep[0].value());
  EXPECT_EQ("", stream.str());
}
