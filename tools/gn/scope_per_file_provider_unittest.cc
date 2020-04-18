// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/scope_per_file_provider.h"
#include "tools/gn/settings.h"
#include "tools/gn/test_with_scope.h"
#include "tools/gn/toolchain.h"
#include "tools/gn/variables.h"

TEST(ScopePerFileProvider, Expected) {
  TestWithScope test;

// Prevent horrible wrapping of calls below.
#define GPV(val) provider.GetProgrammaticValue(val)->string_value()

  // Test the default toolchain.
  {
    Scope scope(test.settings());
    scope.set_source_dir(SourceDir("//source/"));
    ScopePerFileProvider provider(&scope, true);

    EXPECT_EQ("//toolchain:default",    GPV(variables::kCurrentToolchain));
    // TODO(brettw) this test harness does not set up the Toolchain manager
    // which is the source of this value, so we can't test this yet.
    //EXPECT_EQ("//toolchain:default",    GPV(variables::kDefaultToolchain));
    EXPECT_EQ("//out/Debug",            GPV(variables::kRootBuildDir));
    EXPECT_EQ("//out/Debug/gen",        GPV(variables::kRootGenDir));
    EXPECT_EQ("//out/Debug",            GPV(variables::kRootOutDir));
    EXPECT_EQ("//out/Debug/gen/source", GPV(variables::kTargetGenDir));
    EXPECT_EQ("//out/Debug/obj/source", GPV(variables::kTargetOutDir));
  }

  // Test some with an alternate toolchain.
  {
    Settings settings(test.build_settings(), "tc/");
    Toolchain toolchain(&settings, Label(SourceDir("//toolchain/"), "tc"));
    settings.set_toolchain_label(toolchain.label());

    Scope scope(&settings);
    scope.set_source_dir(SourceDir("//source/"));
    ScopePerFileProvider provider(&scope, true);

    EXPECT_EQ("//toolchain:tc",            GPV(variables::kCurrentToolchain));
    // See above.
    //EXPECT_EQ("//toolchain:default",       GPV(variables::kDefaultToolchain));
    EXPECT_EQ("//out/Debug",               GPV(variables::kRootBuildDir));
    EXPECT_EQ("//out/Debug/tc/gen",        GPV(variables::kRootGenDir));
    EXPECT_EQ("//out/Debug/tc",            GPV(variables::kRootOutDir));
    EXPECT_EQ("//out/Debug/tc/gen/source", GPV(variables::kTargetGenDir));
    EXPECT_EQ("//out/Debug/tc/obj/source", GPV(variables::kTargetOutDir));
  }
}
