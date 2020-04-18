// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/target.h"

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/config.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/settings.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"
#include "tools/gn/toolchain.h"

namespace {

// Asserts that the current global scheduler has a single unknown generated
// file with the given name from the given target.
void AssertSchedulerHasOneUnknownFileMatching(const Target* target,
                                              const SourceFile& file) {
  auto unknown = g_scheduler->GetUnknownGeneratedInputs();
  ASSERT_EQ(1u, unknown.size());  // Should be one unknown file.
  auto found = unknown.find(file);
  ASSERT_TRUE(found != unknown.end()) << file.value();
  EXPECT_TRUE(target == found->second)
      << "Target doesn't match. Expected\n  "
      << target->label().GetUserVisibleName(false)
      << "\nBut got\n  " << found->second->label().GetUserVisibleName(false);
}

}  // namespace

using TargetTest = TestWithScheduler;

// Tests that lib[_dir]s are inherited across deps boundaries for static
// libraries but not executables.
TEST_F(TargetTest, LibInheritance) {
  TestWithScope setup;
  Err err;

  const LibFile lib("foo");
  const SourceDir libdir("/foo_dir/");

  // Leaf target with ldflags set.
  TestTarget z(setup, "//foo:z", Target::STATIC_LIBRARY);
  z.config_values().libs().push_back(lib);
  z.config_values().lib_dirs().push_back(libdir);
  ASSERT_TRUE(z.OnResolved(&err));

  // All lib[_dir]s should be set when target is resolved.
  ASSERT_EQ(1u, z.all_libs().size());
  EXPECT_EQ(lib, z.all_libs()[0]);
  ASSERT_EQ(1u, z.all_lib_dirs().size());
  EXPECT_EQ(libdir, z.all_lib_dirs()[0]);

  // Shared library target should inherit the libs from the static library
  // and its own. Its own flag should be before the inherited one.
  const LibFile second_lib("bar");
  const SourceDir second_libdir("/bar_dir/");
  TestTarget shared(setup, "//foo:shared", Target::SHARED_LIBRARY);
  shared.config_values().libs().push_back(second_lib);
  shared.config_values().lib_dirs().push_back(second_libdir);
  shared.private_deps().push_back(LabelTargetPair(&z));
  ASSERT_TRUE(shared.OnResolved(&err));

  ASSERT_EQ(2u, shared.all_libs().size());
  EXPECT_EQ(second_lib, shared.all_libs()[0]);
  EXPECT_EQ(lib, shared.all_libs()[1]);
  ASSERT_EQ(2u, shared.all_lib_dirs().size());
  EXPECT_EQ(second_libdir, shared.all_lib_dirs()[0]);
  EXPECT_EQ(libdir, shared.all_lib_dirs()[1]);

  // Executable target shouldn't get either by depending on shared.
  TestTarget exec(setup, "//foo:exec", Target::EXECUTABLE);
  exec.private_deps().push_back(LabelTargetPair(&shared));
  ASSERT_TRUE(exec.OnResolved(&err));
  EXPECT_EQ(0u, exec.all_libs().size());
  EXPECT_EQ(0u, exec.all_lib_dirs().size());
}

// Test all_dependent_configs and public_config inheritance.
TEST_F(TargetTest, DependentConfigs) {
  TestWithScope setup;
  Err err;

  // Set up a dependency chain of a -> b -> c
  TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
  TestTarget b(setup, "//foo:b", Target::STATIC_LIBRARY);
  TestTarget c(setup, "//foo:c", Target::STATIC_LIBRARY);
  a.private_deps().push_back(LabelTargetPair(&b));
  b.private_deps().push_back(LabelTargetPair(&c));

  // Normal non-inherited config.
  Config config(setup.settings(), Label(SourceDir("//foo/"), "config"));
  ASSERT_TRUE(config.OnResolved(&err));
  c.configs().push_back(LabelConfigPair(&config));

  // All dependent config.
  Config all(setup.settings(), Label(SourceDir("//foo/"), "all"));
  ASSERT_TRUE(all.OnResolved(&err));
  c.all_dependent_configs().push_back(LabelConfigPair(&all));

  // Direct dependent config.
  Config direct(setup.settings(), Label(SourceDir("//foo/"), "direct"));
  ASSERT_TRUE(direct.OnResolved(&err));
  c.public_configs().push_back(LabelConfigPair(&direct));

  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // B should have gotten both dependent configs from C.
  ASSERT_EQ(2u, b.configs().size());
  EXPECT_EQ(&all, b.configs()[0].ptr);
  EXPECT_EQ(&direct, b.configs()[1].ptr);
  ASSERT_EQ(1u, b.all_dependent_configs().size());
  EXPECT_EQ(&all, b.all_dependent_configs()[0].ptr);

  // A should have just gotten the "all" dependent config from C.
  ASSERT_EQ(1u, a.configs().size());
  EXPECT_EQ(&all, a.configs()[0].ptr);
  EXPECT_EQ(&all, a.all_dependent_configs()[0].ptr);

  // Making an an alternate A and B with B forwarding the direct dependents.
  TestTarget a_fwd(setup, "//foo:a_fwd", Target::EXECUTABLE);
  TestTarget b_fwd(setup, "//foo:b_fwd", Target::STATIC_LIBRARY);
  a_fwd.private_deps().push_back(LabelTargetPair(&b_fwd));
  b_fwd.private_deps().push_back(LabelTargetPair(&c));

  ASSERT_TRUE(b_fwd.OnResolved(&err));
  ASSERT_TRUE(a_fwd.OnResolved(&err));

  // A_fwd should now have both configs.
  ASSERT_EQ(1u, a_fwd.configs().size());
  EXPECT_EQ(&all, a_fwd.configs()[0].ptr);
  ASSERT_EQ(1u, a_fwd.all_dependent_configs().size());
  EXPECT_EQ(&all, a_fwd.all_dependent_configs()[0].ptr);
}

// Tests that dependent configs don't propagate between toolchains.
TEST_F(TargetTest, NoDependentConfigsBetweenToolchains) {
  TestWithScope setup;
  Err err;

  // Create another toolchain.
  Toolchain other_toolchain(setup.settings(),
                            Label(SourceDir("//other/"), "toolchain"));
  TestWithScope::SetupToolchain(&other_toolchain);

  // Set up a dependency chain of |a| -> |b| -> |c| where |a| has a different
  // toolchain.
  Target a(setup.settings(),
           Label(SourceDir("//foo/"), "a", other_toolchain.label().dir(),
                 other_toolchain.label().name()));
  a.set_output_type(Target::EXECUTABLE);
  EXPECT_TRUE(a.SetToolchain(&other_toolchain, &err));
  TestTarget b(setup, "//foo:b", Target::EXECUTABLE);
  TestTarget c(setup, "//foo:c", Target::SOURCE_SET);
  a.private_deps().push_back(LabelTargetPair(&b));
  b.private_deps().push_back(LabelTargetPair(&c));

  // All dependent config.
  Config all_dependent(setup.settings(), Label(SourceDir("//foo/"), "all"));
  ASSERT_TRUE(all_dependent.OnResolved(&err));
  c.all_dependent_configs().push_back(LabelConfigPair(&all_dependent));

  // Public config.
  Config public_config(setup.settings(), Label(SourceDir("//foo/"), "public"));
  ASSERT_TRUE(public_config.OnResolved(&err));
  c.public_configs().push_back(LabelConfigPair(&public_config));

  // Another public config.
  Config public_config2(setup.settings(),
                        Label(SourceDir("//foo/"), "public2"));
  ASSERT_TRUE(public_config2.OnResolved(&err));
  b.public_configs().push_back(LabelConfigPair(&public_config2));

  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // B should have gotten the configs from C.
  ASSERT_EQ(3u, b.configs().size());
  EXPECT_EQ(&public_config2, b.configs()[0].ptr);
  EXPECT_EQ(&all_dependent, b.configs()[1].ptr);
  EXPECT_EQ(&public_config, b.configs()[2].ptr);
  ASSERT_EQ(1u, b.all_dependent_configs().size());
  EXPECT_EQ(&all_dependent, b.all_dependent_configs()[0].ptr);

  // A should not have gotten any configs from B or C.
  ASSERT_EQ(0u, a.configs().size());
  ASSERT_EQ(0u, a.all_dependent_configs().size());
}

TEST_F(TargetTest, InheritLibs) {
  TestWithScope setup;
  Err err;

  // Create a dependency chain:
  //   A (executable) -> B (shared lib) -> C (static lib) -> D (source set)
  TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
  TestTarget b(setup, "//foo:b", Target::SHARED_LIBRARY);
  TestTarget c(setup, "//foo:c", Target::STATIC_LIBRARY);
  TestTarget d(setup, "//foo:d", Target::SOURCE_SET);
  a.private_deps().push_back(LabelTargetPair(&b));
  b.private_deps().push_back(LabelTargetPair(&c));
  c.private_deps().push_back(LabelTargetPair(&d));

  ASSERT_TRUE(d.OnResolved(&err));
  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // C should have D in its inherited libs.
  std::vector<const Target*> c_inherited = c.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, c_inherited.size());
  EXPECT_EQ(&d, c_inherited[0]);

  // B should have C and D in its inherited libs.
  std::vector<const Target*> b_inherited = b.inherited_libraries().GetOrdered();
  ASSERT_EQ(2u, b_inherited.size());
  EXPECT_EQ(&c, b_inherited[0]);
  EXPECT_EQ(&d, b_inherited[1]);

  // A should have B in its inherited libs, but not any others (the shared
  // library will include the static library and source set).
  std::vector<const Target*> a_inherited = a.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, a_inherited.size());
  EXPECT_EQ(&b, a_inherited[0]);
}

TEST_F(TargetTest, InheritCompleteStaticLib) {
  TestWithScope setup;
  Err err;

  // Create a dependency chain:
  //   A (executable) -> B (complete static lib) -> C (source set)
  TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
  TestTarget b(setup, "//foo:b", Target::STATIC_LIBRARY);
  b.set_complete_static_lib(true);

  const LibFile lib("foo");
  const SourceDir lib_dir("/foo_dir/");
  TestTarget c(setup, "//foo:c", Target::SOURCE_SET);
  c.config_values().libs().push_back(lib);
  c.config_values().lib_dirs().push_back(lib_dir);

  a.public_deps().push_back(LabelTargetPair(&b));
  b.public_deps().push_back(LabelTargetPair(&c));

  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // B should have C in its inherited libs.
  std::vector<const Target*> b_inherited = b.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, b_inherited.size());
  EXPECT_EQ(&c, b_inherited[0]);

  // A should have B in its inherited libs, but not any others (the complete
  // static library will include the source set).
  std::vector<const Target*> a_inherited = a.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, a_inherited.size());
  EXPECT_EQ(&b, a_inherited[0]);

  // A should inherit the libs and lib_dirs from the C.
  ASSERT_EQ(1u, a.all_libs().size());
  EXPECT_EQ(lib, a.all_libs()[0]);
  ASSERT_EQ(1u, a.all_lib_dirs().size());
  EXPECT_EQ(lib_dir, a.all_lib_dirs()[0]);
}

TEST_F(TargetTest, InheritCompleteStaticLibStaticLibDeps) {
  TestWithScope setup;
  Err err;

  // Create a dependency chain:
  //   A (executable) -> B (complete static lib) -> C (static lib)
  TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
  TestTarget b(setup, "//foo:b", Target::STATIC_LIBRARY);
  b.set_complete_static_lib(true);
  TestTarget c(setup, "//foo:c", Target::STATIC_LIBRARY);
  a.public_deps().push_back(LabelTargetPair(&b));
  b.public_deps().push_back(LabelTargetPair(&c));

  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // B should have C in its inherited libs.
  std::vector<const Target*> b_inherited = b.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, b_inherited.size());
  EXPECT_EQ(&c, b_inherited[0]);

  // A should have B in its inherited libs, but not any others (the complete
  // static library will include the static library).
  std::vector<const Target*> a_inherited = a.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, a_inherited.size());
  EXPECT_EQ(&b, a_inherited[0]);
}

TEST_F(TargetTest, InheritCompleteStaticLibInheritedCompleteStaticLibDeps) {
  TestWithScope setup;
  Err err;

  // Create a dependency chain:
  //   A (executable) -> B (complete static lib) -> C (complete static lib)
  TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
  TestTarget b(setup, "//foo:b", Target::STATIC_LIBRARY);
  b.set_complete_static_lib(true);
  TestTarget c(setup, "//foo:c", Target::STATIC_LIBRARY);
  c.set_complete_static_lib(true);

  a.private_deps().push_back(LabelTargetPair(&b));
  b.private_deps().push_back(LabelTargetPair(&c));

  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(b.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // B should have C in its inherited libs.
  std::vector<const Target*> b_inherited = b.inherited_libraries().GetOrdered();
  ASSERT_EQ(1u, b_inherited.size());
  EXPECT_EQ(&c, b_inherited[0]);

  // A should have B and C in its inherited libs.
  std::vector<const Target*> a_inherited = a.inherited_libraries().GetOrdered();
  ASSERT_EQ(2u, a_inherited.size());
  EXPECT_EQ(&b, a_inherited[0]);
  EXPECT_EQ(&c, a_inherited[1]);
}

TEST_F(TargetTest, NoActionDepPropgation) {
  TestWithScope setup;
  Err err;

  // Create a dependency chain:
  //   A (exe) -> B (action) -> C (source_set)
  {
    TestTarget a(setup, "//foo:a", Target::EXECUTABLE);
    TestTarget b(setup, "//foo:b", Target::ACTION);
    TestTarget c(setup, "//foo:c", Target::SOURCE_SET);

    a.private_deps().push_back(LabelTargetPair(&b));
    b.private_deps().push_back(LabelTargetPair(&c));

    ASSERT_TRUE(c.OnResolved(&err));
    ASSERT_TRUE(b.OnResolved(&err));
    ASSERT_TRUE(a.OnResolved(&err));

    // The executable should not have inherited the source set across the
    // action.
    std::vector<const Target*> libs = a.inherited_libraries().GetOrdered();
    ASSERT_TRUE(libs.empty());
  }
}

TEST_F(TargetTest, GetComputedOutputName) {
  TestWithScope setup;
  Err err;

  // Basic target with no prefix (executable type tool in the TestWithScope has
  // no prefix) or output name.
  TestTarget basic(setup, "//foo:bar", Target::EXECUTABLE);
  ASSERT_TRUE(basic.OnResolved(&err));
  EXPECT_EQ("bar", basic.GetComputedOutputName());

  // Target with no prefix but an output name.
  TestTarget with_name(setup, "//foo:bar", Target::EXECUTABLE);
  with_name.set_output_name("myoutput");
  ASSERT_TRUE(with_name.OnResolved(&err));
  EXPECT_EQ("myoutput", with_name.GetComputedOutputName());

  // Target with a "lib" prefix (the static library tool in the TestWithScope
  // should specify a "lib" output prefix).
  TestTarget with_prefix(setup, "//foo:bar", Target::STATIC_LIBRARY);
  ASSERT_TRUE(with_prefix.OnResolved(&err));
  EXPECT_EQ("libbar", with_prefix.GetComputedOutputName());

  // Target with a "lib" prefix that already has it applied. The prefix should
  // not duplicate something already in the target name.
  TestTarget dup_prefix(setup, "//foo:bar", Target::STATIC_LIBRARY);
  dup_prefix.set_output_name("libbar");
  ASSERT_TRUE(dup_prefix.OnResolved(&err));
  EXPECT_EQ("libbar", dup_prefix.GetComputedOutputName());

  // Target with an output prefix override should not have a prefix.
  TestTarget override_prefix(setup, "//foo:bar", Target::SHARED_LIBRARY);
  override_prefix.set_output_prefix_override(true);
  ASSERT_TRUE(dup_prefix.OnResolved(&err));
  EXPECT_EQ("bar", override_prefix.GetComputedOutputName());
}

// Test visibility failure case.
TEST_F(TargetTest, VisibilityFails) {
  TestWithScope setup;
  Err err;

  TestTarget b(setup, "//private:b", Target::STATIC_LIBRARY);
  b.visibility().SetPrivate(b.label().dir());
  ASSERT_TRUE(b.OnResolved(&err));

  // Make a target depending on "b". The dependency must have an origin to mark
  // it as user-set so we check visibility. This check should fail.
  TestTarget a(setup, "//app:a", Target::EXECUTABLE);
  a.private_deps().push_back(LabelTargetPair(&b));
  IdentifierNode origin;  // Dummy origin.
  a.private_deps()[0].origin = &origin;
  ASSERT_FALSE(a.OnResolved(&err));
}

// Test visibility with a single data_dep.
TEST_F(TargetTest, VisibilityDatadeps) {
  TestWithScope setup;
  Err err;

  TestTarget b(setup, "//public:b", Target::STATIC_LIBRARY);
  ASSERT_TRUE(b.OnResolved(&err));

  // Make a target depending on "b". The dependency must have an origin to mark
  // it as user-set so we check visibility. This check should fail.
  TestTarget a(setup, "//app:a", Target::EXECUTABLE);
  a.data_deps().push_back(LabelTargetPair(&b));
  IdentifierNode origin;  // Dummy origin.
  a.data_deps()[0].origin = &origin;
  ASSERT_TRUE(a.OnResolved(&err)) << err.help_text();
}

// Tests that A -> Group -> B where the group is visible from A but B isn't,
// passes visibility even though the group's deps get expanded into A.
TEST_F(TargetTest, VisibilityGroup) {
  TestWithScope setup;
  Err err;

  IdentifierNode origin;  // Dummy origin.

  // B has private visibility. This lets the group see it since the group is in
  // the same directory.
  TestTarget b(setup, "//private:b", Target::STATIC_LIBRARY);
  b.visibility().SetPrivate(b.label().dir());
  ASSERT_TRUE(b.OnResolved(&err));

  // The group has public visibility and depends on b.
  TestTarget g(setup, "//public:g", Target::GROUP);
  g.private_deps().push_back(LabelTargetPair(&b));
  g.private_deps()[0].origin = &origin;
  ASSERT_TRUE(b.OnResolved(&err));

  // Make a target depending on "g". This should succeed.
  TestTarget a(setup, "//app:a", Target::EXECUTABLE);
  a.private_deps().push_back(LabelTargetPair(&g));
  a.private_deps()[0].origin = &origin;
  ASSERT_TRUE(a.OnResolved(&err));
}

// Verifies that only testonly targets can depend on other testonly targets.
// Many of the above dependency checking cases covered the non-testonly
// case.
TEST_F(TargetTest, Testonly) {
  TestWithScope setup;
  Err err;

  // "testlib" is a test-only library.
  TestTarget testlib(setup, "//test:testlib", Target::STATIC_LIBRARY);
  testlib.set_testonly(true);
  ASSERT_TRUE(testlib.OnResolved(&err));

  // "test" is a test-only executable depending on testlib, this is OK.
  TestTarget test(setup, "//test:test", Target::EXECUTABLE);
  test.set_testonly(true);
  test.private_deps().push_back(LabelTargetPair(&testlib));
  ASSERT_TRUE(test.OnResolved(&err));

  // "product" is a non-test depending on testlib. This should fail.
  TestTarget product(setup, "//app:product", Target::EXECUTABLE);
  product.set_testonly(false);
  product.private_deps().push_back(LabelTargetPair(&testlib));
  ASSERT_FALSE(product.OnResolved(&err));
}

TEST_F(TargetTest, PublicConfigs) {
  TestWithScope setup;
  Err err;

  Label pub_config_label(SourceDir("//a/"), "pubconfig");
  Config pub_config(setup.settings(), pub_config_label);
  LibFile lib_name("testlib");
  pub_config.own_values().libs().push_back(lib_name);
  ASSERT_TRUE(pub_config.OnResolved(&err));

  // This is the destination target that has a public config.
  TestTarget dest(setup, "//a:a", Target::SOURCE_SET);
  dest.public_configs().push_back(LabelConfigPair(&pub_config));
  ASSERT_TRUE(dest.OnResolved(&err));

  // This target has a public dependency on dest.
  TestTarget pub(setup, "//a:pub", Target::SOURCE_SET);
  pub.public_deps().push_back(LabelTargetPair(&dest));
  ASSERT_TRUE(pub.OnResolved(&err));

  // Depending on the target with the public dependency should forward dest's
  // to the current target.
  TestTarget dep_on_pub(setup, "//a:dop", Target::SOURCE_SET);
  dep_on_pub.private_deps().push_back(LabelTargetPair(&pub));
  ASSERT_TRUE(dep_on_pub.OnResolved(&err));
  ASSERT_EQ(1u, dep_on_pub.configs().size());
  EXPECT_EQ(&pub_config, dep_on_pub.configs()[0].ptr);

  // Libs have special handling, check that they were forwarded from the
  // public config to all_libs.
  ASSERT_EQ(1u, dep_on_pub.all_libs().size());
  ASSERT_EQ(lib_name, dep_on_pub.all_libs()[0]);

  // This target has a private dependency on dest for forwards configs.
  TestTarget forward(setup, "//a:f", Target::SOURCE_SET);
  forward.private_deps().push_back(LabelTargetPair(&dest));
  ASSERT_TRUE(forward.OnResolved(&err));
}

// Tests that configs are ordered properly between local and pulled ones.
TEST_F(TargetTest, ConfigOrdering) {
  TestWithScope setup;
  Err err;

  // Make Dep1. It has all_dependent_configs and public_configs.
  TestTarget dep1(setup, "//:dep1", Target::SOURCE_SET);
  Label dep1_all_config_label(SourceDir("//"), "dep1_all_config");
  Config dep1_all_config(setup.settings(), dep1_all_config_label);
  ASSERT_TRUE(dep1_all_config.OnResolved(&err));
  dep1.all_dependent_configs().push_back(LabelConfigPair(&dep1_all_config));

  Label dep1_public_config_label(SourceDir("//"), "dep1_public_config");
  Config dep1_public_config(setup.settings(), dep1_public_config_label);
  ASSERT_TRUE(dep1_public_config.OnResolved(&err));
  dep1.public_configs().push_back(LabelConfigPair(&dep1_public_config));
  ASSERT_TRUE(dep1.OnResolved(&err));

  // Make Dep2 with the same structure.
  TestTarget dep2(setup, "//:dep2", Target::SOURCE_SET);
  Label dep2_all_config_label(SourceDir("//"), "dep2_all_config");
  Config dep2_all_config(setup.settings(), dep2_all_config_label);
  ASSERT_TRUE(dep2_all_config.OnResolved(&err));
  dep2.all_dependent_configs().push_back(LabelConfigPair(&dep2_all_config));

  Label dep2_public_config_label(SourceDir("//"), "dep2_public_config");
  Config dep2_public_config(setup.settings(), dep2_public_config_label);
  ASSERT_TRUE(dep2_public_config.OnResolved(&err));
  dep2.public_configs().push_back(LabelConfigPair(&dep2_public_config));
  ASSERT_TRUE(dep2.OnResolved(&err));

  // This target depends on both previous targets.
  TestTarget target(setup, "//:foo", Target::SOURCE_SET);
  target.private_deps().push_back(LabelTargetPair(&dep1));
  target.private_deps().push_back(LabelTargetPair(&dep2));

  // It also has a private and public config.
  Label public_config_label(SourceDir("//"), "public");
  Config public_config(setup.settings(), public_config_label);
  ASSERT_TRUE(public_config.OnResolved(&err));
  target.public_configs().push_back(LabelConfigPair(&public_config));

  Label private_config_label(SourceDir("//"), "private");
  Config private_config(setup.settings(), private_config_label);
  ASSERT_TRUE(private_config.OnResolved(&err));
  target.configs().push_back(LabelConfigPair(&private_config));

  // Resolve to get the computed list of configs applying.
  ASSERT_TRUE(target.OnResolved(&err));
  const auto& computed = target.configs();

  // Order should be:
  // 1. local private
  // 2. local public
  // 3. inherited all dependent
  // 4. inherited public
  ASSERT_EQ(6u, computed.size());
  EXPECT_EQ(private_config_label, computed[0].label);
  EXPECT_EQ(public_config_label, computed[1].label);
  EXPECT_EQ(dep1_all_config_label, computed[2].label);
  EXPECT_EQ(dep2_all_config_label, computed[3].label);
  EXPECT_EQ(dep1_public_config_label, computed[4].label);
  EXPECT_EQ(dep2_public_config_label, computed[5].label);
}

// Tests that different link/depend outputs work for solink tools.
TEST_F(TargetTest, LinkAndDepOutputs) {
  TestWithScope setup;
  Err err;

  Toolchain toolchain(setup.settings(), Label(SourceDir("//tc/"), "tc"));

  std::unique_ptr<Tool> solink_tool = std::make_unique<Tool>();
  solink_tool->set_output_prefix("lib");
  solink_tool->set_default_output_extension(".so");

  const char kLinkPattern[] =
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}";
  SubstitutionPattern link_output = SubstitutionPattern::MakeForTest(
      kLinkPattern);
  solink_tool->set_link_output(link_output);

  const char kDependPattern[] =
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}.TOC";
  SubstitutionPattern depend_output = SubstitutionPattern::MakeForTest(
      kDependPattern);
  solink_tool->set_depend_output(depend_output);

  solink_tool->set_outputs(SubstitutionList::MakeForTest(
      kLinkPattern, kDependPattern));

  toolchain.SetTool(Toolchain::TYPE_SOLINK, std::move(solink_tool));

  Target target(setup.settings(), Label(SourceDir("//a/"), "a"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.SetToolchain(&toolchain);
  ASSERT_TRUE(target.OnResolved(&err));

  EXPECT_EQ("./liba.so", target.link_output_file().value());
  EXPECT_EQ("./liba.so.TOC", target.dependency_output_file().value());

  ASSERT_EQ(1u, target.runtime_outputs().size());
  EXPECT_EQ("./liba.so", target.runtime_outputs()[0].value());
}

// Tests that runtime_outputs works without an explicit link_output for
// solink tools.
TEST_F(TargetTest, RuntimeOuputs) {
  TestWithScope setup;
  Err err;

  Toolchain toolchain(setup.settings(), Label(SourceDir("//tc/"), "tc"));

  std::unique_ptr<Tool> solink_tool = std::make_unique<Tool>();
  solink_tool->set_output_prefix("");
  solink_tool->set_default_output_extension(".dll");

  // Say the linker makes a DLL< an import library, and a symbol file we want
  // to treat as a runtime output.
  const char kLibPattern[] =
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}.lib";
  const char kDllPattern[] =
      "{{root_out_dir}}/{{target_output_name}}{{output_extension}}";
  const char kPdbPattern[] =
      "{{root_out_dir}}/{{target_output_name}}.pdb";
  SubstitutionPattern pdb_pattern =
      SubstitutionPattern::MakeForTest(kPdbPattern);

  solink_tool->set_outputs(
      SubstitutionList::MakeForTest(kLibPattern, kDllPattern, kPdbPattern));

  // Say we only want the DLL and symbol file treaded as runtime outputs.
  solink_tool->set_runtime_outputs(SubstitutionList::MakeForTest(
      kDllPattern, kPdbPattern));

  toolchain.SetTool(Toolchain::TYPE_SOLINK, std::move(solink_tool));

  Target target(setup.settings(), Label(SourceDir("//a/"), "a"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.SetToolchain(&toolchain);
  ASSERT_TRUE(target.OnResolved(&err));

  EXPECT_EQ("./a.dll.lib", target.link_output_file().value());
  EXPECT_EQ("./a.dll.lib", target.dependency_output_file().value());

  ASSERT_EQ(2u, target.runtime_outputs().size());
  EXPECT_EQ("./a.dll", target.runtime_outputs()[0].value());
  EXPECT_EQ("./a.pdb", target.runtime_outputs()[1].value());
}

// Shared libraries should be inherited across public shared liobrary
// boundaries.
TEST_F(TargetTest, SharedInheritance) {
  TestWithScope setup;
  Err err;

  // Create two leaf shared libraries.
  TestTarget pub(setup, "//foo:pub", Target::SHARED_LIBRARY);
  ASSERT_TRUE(pub.OnResolved(&err));

  TestTarget priv(setup, "//foo:priv", Target::SHARED_LIBRARY);
  ASSERT_TRUE(priv.OnResolved(&err));

  // Intermediate shared library with the leaf shared libraries as
  // dependencies, one public, one private.
  TestTarget inter(setup, "//foo:inter", Target::SHARED_LIBRARY);
  inter.public_deps().push_back(LabelTargetPair(&pub));
  inter.private_deps().push_back(LabelTargetPair(&priv));
  ASSERT_TRUE(inter.OnResolved(&err));

  // The intermediate shared library should have both "pub" and "priv" in its
  // inherited libraries.
  std::vector<const Target*> inter_inherited =
      inter.inherited_libraries().GetOrdered();
  ASSERT_EQ(2u, inter_inherited.size());
  EXPECT_EQ(&pub, inter_inherited[0]);
  EXPECT_EQ(&priv, inter_inherited[1]);

  // Make a toplevel executable target depending on the intermediate one.
  TestTarget exe(setup, "//foo:exe", Target::SHARED_LIBRARY);
  exe.private_deps().push_back(LabelTargetPair(&inter));
  ASSERT_TRUE(exe.OnResolved(&err));

  // The exe's inherited libraries should be "inter" (because it depended
  // directly on it) and "pub" (because inter depended publicly on it).
  std::vector<const Target*> exe_inherited =
      exe.inherited_libraries().GetOrdered();
  ASSERT_EQ(2u, exe_inherited.size());
  EXPECT_EQ(&inter, exe_inherited[0]);
  EXPECT_EQ(&pub, exe_inherited[1]);
}

TEST_F(TargetTest, GeneratedInputs) {
  TestWithScope setup;
  Err err;

  SourceFile generated_file("//out/Debug/generated.cc");

  // This target has a generated input and no dependency makes it.
  TestTarget non_existent_generator(setup, "//foo:non_existent_generator",
                                    Target::EXECUTABLE);
  non_existent_generator.sources().push_back(generated_file);
  EXPECT_TRUE(non_existent_generator.OnResolved(&err)) << err.message();
  AssertSchedulerHasOneUnknownFileMatching(&non_existent_generator,
                                           generated_file);
  scheduler().ClearUnknownGeneratedInputsAndWrittenFiles();

  // Make a target that generates the file.
  TestTarget generator(setup, "//foo:generator", Target::ACTION);
  generator.action_values().outputs() =
      SubstitutionList::MakeForTest(generated_file.value().c_str());
  err = Err();
  EXPECT_TRUE(generator.OnResolved(&err)) << err.message();

  // A target that depends on the generator that uses the file as a source
  // should be OK. This uses a private dep (will be used later).
  TestTarget existent_generator(setup, "//foo:existent_generator",
                                Target::SHARED_LIBRARY);
  existent_generator.sources().push_back(generated_file);
  existent_generator.private_deps().push_back(LabelTargetPair(&generator));
  EXPECT_TRUE(existent_generator.OnResolved(&err)) << err.message();
  EXPECT_TRUE(scheduler().GetUnknownGeneratedInputs().empty());

  // A target that depends on the previous one should *not* be allowed to
  // use the generated file, because existent_generator used private deps.
  // This is:
  //    indirect_private --> existent_generator --[private]--> generator
  TestTarget indirect_private(setup, "//foo:indirect_private",
                              Target::EXECUTABLE);
  indirect_private.sources().push_back(generated_file);
  indirect_private.public_deps().push_back(
      LabelTargetPair(&existent_generator));
  EXPECT_TRUE(indirect_private.OnResolved(&err));
  AssertSchedulerHasOneUnknownFileMatching(&indirect_private, generated_file);
  scheduler().ClearUnknownGeneratedInputsAndWrittenFiles();

  // Now make a chain like the above but with all public deps, it should be OK.
  TestTarget existent_public(setup, "//foo:existent_public",
                             Target::SHARED_LIBRARY);
  existent_public.public_deps().push_back(LabelTargetPair(&generator));
  EXPECT_TRUE(existent_public.OnResolved(&err)) << err.message();
  TestTarget indirect_public(setup, "//foo:indirect_public",
                             Target::EXECUTABLE);
  indirect_public.sources().push_back(generated_file);
  indirect_public.public_deps().push_back(LabelTargetPair(&existent_public));
  EXPECT_TRUE(indirect_public.OnResolved(&err)) << err.message();
  EXPECT_TRUE(scheduler().GetUnknownGeneratedInputs().empty());
}

// This is sort of a Scheduler test, but is related to the above test more.
TEST_F(TargetTest, WriteFileGeneratedInputs) {
  TestWithScope setup;
  Err err;

  SourceFile generated_file("//out/Debug/generated.data");

  // This target has a generated input and no dependency makes it.
  TestTarget non_existent_generator(setup, "//foo:non_existent_generator",
                                    Target::EXECUTABLE);
  non_existent_generator.sources().push_back(generated_file);
  EXPECT_TRUE(non_existent_generator.OnResolved(&err));
  AssertSchedulerHasOneUnknownFileMatching(&non_existent_generator,
                                           generated_file);
  scheduler().ClearUnknownGeneratedInputsAndWrittenFiles();

  // This target has a generated file and we've decared we write it.
  TestTarget existent_generator(setup, "//foo:existent_generator",
                                Target::EXECUTABLE);
  existent_generator.sources().push_back(generated_file);
  EXPECT_TRUE(existent_generator.OnResolved(&err));
  scheduler().AddWrittenFile(generated_file);

  // Should be OK.
  EXPECT_TRUE(scheduler().GetUnknownGeneratedInputs().empty());
}

TEST_F(TargetTest, WriteRuntimeDepsGeneratedInputs) {
  TestWithScope setup;
  Err err;

  SourceFile source_file("//out/Debug/generated.runtime_deps");
  OutputFile output_file(setup.build_settings(), source_file);

  TestTarget generator(setup, "//foo:generator", Target::EXECUTABLE);
  generator.set_write_runtime_deps_output(output_file);
  g_scheduler->AddWriteRuntimeDepsTarget(&generator);

  TestTarget middle_data_dep(setup, "//foo:middle", Target::EXECUTABLE);
  middle_data_dep.data_deps().push_back(LabelTargetPair(&generator));

  // This target has a generated input and no dependency makes it.
  TestTarget dep_missing(setup, "//foo:no_dep", Target::EXECUTABLE);
  dep_missing.sources().push_back(source_file);
  EXPECT_TRUE(dep_missing.OnResolved(&err));
  AssertSchedulerHasOneUnknownFileMatching(&dep_missing, source_file);
  scheduler().ClearUnknownGeneratedInputsAndWrittenFiles();

  // This target has a generated file and we've directly dependended on it.
  TestTarget dep_present(setup, "//foo:with_dep", Target::EXECUTABLE);
  dep_present.sources().push_back(source_file);
  dep_present.private_deps().push_back(LabelTargetPair(&generator));
  EXPECT_TRUE(dep_present.OnResolved(&err));
  EXPECT_TRUE(scheduler().GetUnknownGeneratedInputs().empty());

  // This target has a generated file and we've indirectly dependended on it
  // via data_deps.
  TestTarget dep_indirect(setup, "//foo:with_dep", Target::EXECUTABLE);
  dep_indirect.sources().push_back(source_file);
  dep_indirect.data_deps().push_back(LabelTargetPair(&middle_data_dep));
  EXPECT_TRUE(dep_indirect.OnResolved(&err));
  AssertSchedulerHasOneUnknownFileMatching(&dep_indirect, source_file);
  scheduler().ClearUnknownGeneratedInputsAndWrittenFiles();

  // This target has a generated file and we've directly dependended on it
  // via data_deps.
  TestTarget data_dep_present(setup, "//foo:with_dep", Target::EXECUTABLE);
  data_dep_present.sources().push_back(source_file);
  data_dep_present.data_deps().push_back(LabelTargetPair(&generator));
  EXPECT_TRUE(data_dep_present.OnResolved(&err));
  EXPECT_TRUE(scheduler().GetUnknownGeneratedInputs().empty());
}

// Tests that intermediate object files generated by binary targets are also
// considered generated for the purposes of input checking. Above, we tested
// the failure cases for generated inputs, so here only test .o files that are
// present.
TEST_F(TargetTest, ObjectGeneratedInputs) {
  TestWithScope setup;
  Err err;

  // This target compiles the source.
  SourceFile source_file("//source.cc");
  TestTarget source_generator(setup, "//:source_target", Target::SOURCE_SET);
  source_generator.sources().push_back(source_file);
  EXPECT_TRUE(source_generator.OnResolved(&err));

  // This is the object file that the test toolchain generates for the source.
  SourceFile object_file("//out/Debug/obj/source_target.source.o");

  TestTarget final_target(setup, "//:final", Target::ACTION);
  final_target.config_values().inputs().push_back(object_file);
  EXPECT_TRUE(final_target.OnResolved(&err));

  AssertSchedulerHasOneUnknownFileMatching(&final_target, object_file);
}

TEST_F(TargetTest, ResolvePrecompiledHeaders) {
  TestWithScope setup;
  Err err;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));

  // Target with no settings, no configs, should be a no-op.
  EXPECT_TRUE(target.ResolvePrecompiledHeaders(&err));

  // Config with PCH values.
  Config config_1(setup.settings(), Label(SourceDir("//foo/"), "c1"));
  std::string pch_1("pch.h");
  SourceFile pcs_1("//pcs.cc");
  config_1.own_values().set_precompiled_header(pch_1);
  config_1.own_values().set_precompiled_source(pcs_1);
  ASSERT_TRUE(config_1.OnResolved(&err));
  target.configs().push_back(LabelConfigPair(&config_1));

  // No PCH info specified on TargetTest, but the config specifies one, the
  // values should get copied to the target.
  EXPECT_TRUE(target.ResolvePrecompiledHeaders(&err));
  EXPECT_EQ(pch_1, target.config_values().precompiled_header());
  EXPECT_TRUE(target.config_values().precompiled_source() == pcs_1);

  // Now both target and config have matching PCH values. Resolving again
  // should be a no-op since they all match.
  EXPECT_TRUE(target.ResolvePrecompiledHeaders(&err));
  EXPECT_TRUE(target.config_values().precompiled_header() == pch_1);
  EXPECT_TRUE(target.config_values().precompiled_source() == pcs_1);

  // Second config with different PCH values.
  Config config_2(setup.settings(), Label(SourceDir("//foo/"), "c2"));
  std::string pch_2("pch2.h");
  SourceFile pcs_2("//pcs2.cc");
  config_2.own_values().set_precompiled_header(pch_2);
  config_2.own_values().set_precompiled_source(pcs_2);
  ASSERT_TRUE(config_2.OnResolved(&err));
  target.configs().push_back(LabelConfigPair(&config_2));

  // This should be an error since they don't match.
  EXPECT_FALSE(target.ResolvePrecompiledHeaders(&err));

  // Make sure the proper labels are blamed.
  EXPECT_EQ(
      "The target //foo:bar\n"
      "has conflicting precompiled header settings.\n"
      "\n"
      "From //foo:bar\n"
      "  header: pch.h\n"
      "  source: //pcs.cc\n"
      "\n"
      "From //foo:c2\n"
      "  header: pch2.h\n"
      "  source: //pcs2.cc",
      err.help_text());
}

TEST_F(TargetTest, AssertNoDeps) {
  TestWithScope setup;
  Err err;

  // A target.
  TestTarget a(setup, "//a", Target::SHARED_LIBRARY);
  ASSERT_TRUE(a.OnResolved(&err));

  // B depends on A and has an assert_no_deps for a random dir.
  TestTarget b(setup, "//b", Target::SHARED_LIBRARY);
  b.private_deps().push_back(LabelTargetPair(&a));
  b.assert_no_deps().push_back(LabelPattern(
      LabelPattern::RECURSIVE_DIRECTORY, SourceDir("//disallowed/"),
      std::string(), Label()));
  ASSERT_TRUE(b.OnResolved(&err));

  LabelPattern disallow_a(LabelPattern::RECURSIVE_DIRECTORY, SourceDir("//a/"),
                          std::string(), Label());

  // C depends on B and disallows depending on A. This should fail.
  TestTarget c(setup, "//c", Target::EXECUTABLE);
  c.private_deps().push_back(LabelTargetPair(&b));
  c.assert_no_deps().push_back(disallow_a);
  ASSERT_FALSE(c.OnResolved(&err));

  // Validate the error message has the proper path.
  EXPECT_EQ(
      "//c:c has an assert_no_deps entry:\n"
      "  //a/*\n"
      "which fails for the dependency path:\n"
      "  //c:c ->\n"
      "  //b:b ->\n"
      "  //a:a",
      err.help_text());
  err = Err();

  // Add an intermediate executable with: exe -> b -> a
  TestTarget exe(setup, "//exe", Target::EXECUTABLE);
  exe.private_deps().push_back(LabelTargetPair(&b));
  ASSERT_TRUE(exe.OnResolved(&err));

  // D depends on the executable and disallows depending on A. Since there is
  // an intermediate executable, this should be OK.
  TestTarget d(setup, "//d", Target::EXECUTABLE);
  d.private_deps().push_back(LabelTargetPair(&exe));
  d.assert_no_deps().push_back(disallow_a);
  ASSERT_TRUE(d.OnResolved(&err));

  // A2 disallows depending on anything in its own directory, but the
  // assertions should not match the target itself so this should be OK.
  TestTarget a2(setup, "//a:a2", Target::EXECUTABLE);
  a2.assert_no_deps().push_back(disallow_a);
  ASSERT_TRUE(a2.OnResolved(&err));
}

TEST_F(TargetTest, PullRecursiveBundleData) {
  TestWithScope setup;
  Err err;

  // We have the following dependency graph:
  // A (create_bundle) -> B (bundle_data)
  //                  \-> C (create_bundle) -> D (bundle_data)
  //                  \-> E (group) -> F (bundle_data)
  //                               \-> B (bundle_data)
  TestTarget a(setup, "//foo:a", Target::CREATE_BUNDLE);
  TestTarget b(setup, "//foo:b", Target::BUNDLE_DATA);
  TestTarget c(setup, "//foo:c", Target::CREATE_BUNDLE);
  TestTarget d(setup, "//foo:d", Target::BUNDLE_DATA);
  TestTarget e(setup, "//foo:e", Target::GROUP);
  TestTarget f(setup, "//foo:f", Target::BUNDLE_DATA);
  a.public_deps().push_back(LabelTargetPair(&b));
  a.public_deps().push_back(LabelTargetPair(&c));
  a.public_deps().push_back(LabelTargetPair(&e));
  c.public_deps().push_back(LabelTargetPair(&d));
  e.public_deps().push_back(LabelTargetPair(&f));
  e.public_deps().push_back(LabelTargetPair(&b));

  b.sources().push_back(SourceFile("//foo/b1.txt"));
  b.sources().push_back(SourceFile("//foo/b2.txt"));
  b.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  ASSERT_TRUE(b.OnResolved(&err));

  d.sources().push_back(SourceFile("//foo/d.txt"));
  d.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  ASSERT_TRUE(d.OnResolved(&err));

  f.sources().push_back(SourceFile("//foo/f1.txt"));
  f.sources().push_back(SourceFile("//foo/f2.txt"));
  f.sources().push_back(SourceFile("//foo/f3.txt"));
  f.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/Contents.json"));
  f.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooEmpty-29.png"));
  f.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooEmpty-29@2x.png"));
  f.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooEmpty-29@3x.png"));
  f.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  ASSERT_TRUE(f.OnResolved(&err));

  ASSERT_TRUE(e.OnResolved(&err));
  ASSERT_TRUE(c.OnResolved(&err));
  ASSERT_TRUE(a.OnResolved(&err));

  // A gets its data from B and F.
  ASSERT_EQ(a.bundle_data().file_rules().size(), 2u);
  ASSERT_EQ(a.bundle_data().file_rules()[0].sources().size(), 2u);
  ASSERT_EQ(a.bundle_data().file_rules()[1].sources().size(), 3u);
  ASSERT_EQ(a.bundle_data().assets_catalog_sources().size(), 1u);
  ASSERT_EQ(a.bundle_data().bundle_deps().size(), 2u);

  // C gets its data from D.
  ASSERT_EQ(c.bundle_data().file_rules().size(), 1u);
  ASSERT_EQ(c.bundle_data().file_rules()[0].sources().size(), 1u);
  ASSERT_EQ(c.bundle_data().bundle_deps().size(), 1u);

  // E does not have any bundle_data information but gets a list of
  // bundle_deps to propagate them during target resolution.
  ASSERT_TRUE(e.bundle_data().file_rules().empty());
  ASSERT_TRUE(e.bundle_data().assets_catalog_sources().empty());
  ASSERT_EQ(e.bundle_data().bundle_deps().size(), 2u);
}
