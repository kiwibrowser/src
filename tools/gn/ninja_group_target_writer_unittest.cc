// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/ninja_group_target_writer.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

TEST(NinjaGroupTargetWriter, Run) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::GROUP);
  target.visibility().SetPublic();

  Target dep(setup.settings(), Label(SourceDir("//foo/"), "dep"));
  dep.set_output_type(Target::ACTION);
  dep.visibility().SetPublic();
  dep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep.OnResolved(&err));

  Target dep2(setup.settings(), Label(SourceDir("//foo/"), "dep2"));
  dep2.set_output_type(Target::ACTION);
  dep2.visibility().SetPublic();
  dep2.SetToolchain(setup.toolchain());
  ASSERT_TRUE(dep2.OnResolved(&err));

  Target datadep(setup.settings(), Label(SourceDir("//foo/"), "datadep"));
  datadep.set_output_type(Target::ACTION);
  datadep.visibility().SetPublic();
  datadep.SetToolchain(setup.toolchain());
  ASSERT_TRUE(datadep.OnResolved(&err));

  target.public_deps().push_back(LabelTargetPair(&dep));
  target.public_deps().push_back(LabelTargetPair(&dep2));
  target.data_deps().push_back(LabelTargetPair(&datadep));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaGroupTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "build obj/foo/bar.stamp: stamp obj/foo/dep.stamp obj/foo/dep2.stamp || obj/foo/datadep.stamp\n";
  EXPECT_EQ(expected, out.str());
}
