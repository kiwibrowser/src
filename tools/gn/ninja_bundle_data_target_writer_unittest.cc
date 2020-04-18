// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_bundle_data_target_writer.h"

#include <algorithm>
#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

TEST(NinjaBundleDataTargetWriter, Run) {
  Err err;
  TestWithScope setup;

  Target bundle_data(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data.set_output_type(Target::BUNDLE_DATA);
  bundle_data.sources().push_back(SourceFile("//foo/input1.txt"));
  bundle_data.sources().push_back(SourceFile("//foo/input2.txt"));
  bundle_data.sources().push_back(
      SourceFile("//foo/Foo.xcassets/Contents.json"));
  bundle_data.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/Contents.json"));
  bundle_data.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29.png"));
  bundle_data.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29@2x.png"));
  bundle_data.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29@3x.png"));
  bundle_data.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data.SetToolchain(setup.toolchain());
  bundle_data.visibility().SetPublic();
  ASSERT_TRUE(bundle_data.OnResolved(&err));

  std::ostringstream out;
  NinjaBundleDataTargetWriter writer(&bundle_data, out);
  writer.Run();

  const char expected[] =
      "build obj/foo/data.stamp: stamp "
          "../../foo/input1.txt "
          "../../foo/input2.txt "
          "../../foo/Foo.xcassets/Contents.json "
          "../../foo/Foo.xcassets/foo.imageset/Contents.json "
          "../../foo/Foo.xcassets/foo.imageset/FooIcon-29.png "
          "../../foo/Foo.xcassets/foo.imageset/FooIcon-29@2x.png "
          "../../foo/Foo.xcassets/foo.imageset/FooIcon-29@3x.png\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}
