// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_create_bundle_target_writer.h"

#include <algorithm>
#include <memory>
#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

namespace {

void SetupBundleDataDir(BundleData* bundle_data, const std::string& root_dir) {
  std::string bundle_root_dir = root_dir + "/bar.bundle";
  bundle_data->root_dir() = SourceDir(bundle_root_dir);
  bundle_data->contents_dir() = SourceDir(bundle_root_dir + "/Contents");
  bundle_data->resources_dir() =
      SourceDir(bundle_data->contents_dir().value() + "/Resources");
  bundle_data->executable_dir() =
      SourceDir(bundle_data->contents_dir().value() + "/MacOS");
  bundle_data->plugins_dir() =
      SourceDir(bundle_data->contents_dir().value() + "/Plug Ins");
}

std::unique_ptr<Target> NewAction(const TestWithScope& setup) {
  Err err;
  auto action = std::make_unique<Target>(setup.settings(),
                                         Label(SourceDir("//foo/"), "bar"));
  action->set_output_type(Target::ACTION);
  action->visibility().SetPublic();
  action->action_values().set_script(SourceFile("//foo/script.py"));

  action->action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/foo.out");

  action->SetToolchain(setup.toolchain());
  return action;
}

}  // namespace

// Tests multiple files with an output pattern.
TEST(NinjaCreateBundleTargetWriter, Run) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target bundle_data(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data.set_output_type(Target::BUNDLE_DATA);
  bundle_data.sources().push_back(SourceFile("//foo/input1.txt"));
  bundle_data.sources().push_back(SourceFile("//foo/input2.txt"));
  bundle_data.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data.SetToolchain(setup.toolchain());
  bundle_data.visibility().SetPublic();
  ASSERT_TRUE(bundle_data.OnResolved(&err));

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data));
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.inputdeps.stamp: stamp obj/foo/bar.stamp "
      "obj/foo/data.stamp\n"
      "build bar.bundle/Contents/Resources/input1.txt: copy_bundle_data "
      "../../foo/input1.txt || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle/Contents/Resources/input2.txt: copy_bundle_data "
      "../../foo/input2.txt || obj/baz/bar.inputdeps.stamp\n"
      "build obj/baz/bar.stamp: stamp "
      "bar.bundle/Contents/Resources/input1.txt "
      "bar.bundle/Contents/Resources/input2.txt"
      " || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

// Tests creating a bundle in a sub-directory of $root_out_dir.
TEST(NinjaCreateBundleTargetWriter, InSubDirectory) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target bundle_data(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data.set_output_type(Target::BUNDLE_DATA);
  bundle_data.sources().push_back(SourceFile("//foo/input1.txt"));
  bundle_data.sources().push_back(SourceFile("//foo/input2.txt"));
  bundle_data.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data.SetToolchain(setup.toolchain());
  bundle_data.visibility().SetPublic();
  ASSERT_TRUE(bundle_data.OnResolved(&err));

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug/gen");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data));
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.inputdeps.stamp: stamp obj/foo/bar.stamp "
      "obj/foo/data.stamp\n"
      "build gen/bar.bundle/Contents/Resources/input1.txt: copy_bundle_data "
      "../../foo/input1.txt || obj/baz/bar.inputdeps.stamp\n"
      "build gen/bar.bundle/Contents/Resources/input2.txt: copy_bundle_data "
      "../../foo/input2.txt || obj/baz/bar.inputdeps.stamp\n"
      "build obj/baz/bar.stamp: stamp "
      "gen/bar.bundle/Contents/Resources/input1.txt "
      "gen/bar.bundle/Contents/Resources/input2.txt || "
      "obj/baz/bar.inputdeps.stamp\n"
      "build gen/bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

// Tests empty asset catalog with partial_info_plist property defined.
TEST(NinjaCreateBundleTargetWriter, JustPartialInfoPlist) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.bundle_data().product_type().assign("com.apple.product-type");
  create_bundle.bundle_data().set_partial_info_plist(
      SourceFile("//out/Debug/baz/bar/bar_partial_info.plist"));
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build baz/bar/bar_partial_info.plist: stamp || obj/foo/bar.stamp\n"
      "build obj/baz/bar.stamp: stamp "
      "baz/bar/bar_partial_info.plist || obj/foo/bar.stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}


// Tests multiple files from asset catalog.
TEST(NinjaCreateBundleTargetWriter, AssetCatalog) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target bundle_data(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data.set_output_type(Target::BUNDLE_DATA);
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

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data));
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.bundle_data().product_type().assign("com.apple.product-type");
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.inputdeps.stamp: stamp obj/foo/bar.stamp "
      "obj/foo/data.stamp\n"
      "build bar.bundle/Contents/Resources/Assets.car: compile_xcassets "
      "../../foo/Foo.xcassets | obj/foo/data.stamp || "
      "obj/baz/bar.inputdeps.stamp\n"
      "  product_type = com.apple.product-type\n"
      "build obj/baz/bar.stamp: stamp "
      "bar.bundle/Contents/Resources/Assets.car || "
      "obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

// Tests that the phony target for the top-level bundle directory is generated
// correctly.
TEST(NinjaCreateBundleTargetWriter, PhonyTarget) {
  Err err;
  TestWithScope setup;

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.stamp: stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

// Tests complex target with multiple bundle_data sources, including
// some asset catalog.
TEST(NinjaCreateBundleTargetWriter, Complex) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target bundle_data0(setup.settings(),
                      Label(SourceDir("//qux/"), "info_plist"));
  bundle_data0.set_output_type(Target::BUNDLE_DATA);
  bundle_data0.sources().push_back(SourceFile("//qux/qux-Info.plist"));
  bundle_data0.action_values().outputs() =
      SubstitutionList::MakeForTest("{{bundle_contents_dir}}/Info.plist");
  bundle_data0.SetToolchain(setup.toolchain());
  bundle_data0.visibility().SetPublic();
  ASSERT_TRUE(bundle_data0.OnResolved(&err));

  Target bundle_data1(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data1.set_output_type(Target::BUNDLE_DATA);
  bundle_data1.sources().push_back(SourceFile("//foo/input1.txt"));
  bundle_data1.sources().push_back(SourceFile("//foo/input2.txt"));
  bundle_data1.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data1.SetToolchain(setup.toolchain());
  bundle_data1.visibility().SetPublic();
  ASSERT_TRUE(bundle_data1.OnResolved(&err));

  Target bundle_data2(setup.settings(), Label(SourceDir("//foo/"), "assets"));
  bundle_data2.set_output_type(Target::BUNDLE_DATA);
  bundle_data2.sources().push_back(
      SourceFile("//foo/Foo.xcassets/Contents.json"));
  bundle_data2.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/Contents.json"));
  bundle_data2.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29.png"));
  bundle_data2.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29@2x.png"));
  bundle_data2.sources().push_back(
      SourceFile("//foo/Foo.xcassets/foo.imageset/FooIcon-29@3x.png"));
  bundle_data2.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data2.SetToolchain(setup.toolchain());
  bundle_data2.visibility().SetPublic();
  ASSERT_TRUE(bundle_data2.OnResolved(&err));

  Target bundle_data3(setup.settings(), Label(SourceDir("//quz/"), "assets"));
  bundle_data3.set_output_type(Target::BUNDLE_DATA);
  bundle_data3.sources().push_back(
      SourceFile("//quz/Quz.xcassets/Contents.json"));
  bundle_data3.sources().push_back(
      SourceFile("//quz/Quz.xcassets/quz.imageset/Contents.json"));
  bundle_data3.sources().push_back(
      SourceFile("//quz/Quz.xcassets/quz.imageset/QuzIcon-29.png"));
  bundle_data3.sources().push_back(
      SourceFile("//quz/Quz.xcassets/quz.imageset/QuzIcon-29@2x.png"));
  bundle_data3.sources().push_back(
      SourceFile("//quz/Quz.xcassets/quz.imageset/QuzIcon-29@3x.png"));
  bundle_data3.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data3.SetToolchain(setup.toolchain());
  bundle_data3.visibility().SetPublic();
  ASSERT_TRUE(bundle_data3.OnResolved(&err));

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data0));
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data1));
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data2));
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data3));
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.bundle_data().product_type().assign("com.apple.product-type");
  create_bundle.bundle_data().set_partial_info_plist(
      SourceFile("//out/Debug/baz/bar/bar_partial_info.plist"));
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.inputdeps.stamp: stamp obj/foo/assets.stamp "
      "obj/foo/bar.stamp obj/foo/data.stamp obj/qux/info_plist.stamp "
      "obj/quz/assets.stamp\n"
      "build bar.bundle/Contents/Info.plist: copy_bundle_data "
      "../../qux/qux-Info.plist || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle/Contents/Resources/input1.txt: copy_bundle_data "
      "../../foo/input1.txt || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle/Contents/Resources/input2.txt: copy_bundle_data "
      "../../foo/input2.txt || obj/baz/bar.inputdeps.stamp\n"
      "build obj/baz/bar.xcassets.inputdeps.stamp: stamp "
      "obj/foo/assets.stamp "
      "obj/quz/assets.stamp\n"
      "build bar.bundle/Contents/Resources/Assets.car | "
      "baz/bar/bar_partial_info.plist: compile_xcassets "
      "../../foo/Foo.xcassets "
      "../../quz/Quz.xcassets | obj/baz/bar.xcassets.inputdeps.stamp || "
      "obj/baz/bar.inputdeps.stamp\n"
      "  product_type = com.apple.product-type\n"
      "  partial_info_plist = baz/bar/bar_partial_info.plist\n"
      "build obj/baz/bar.stamp: stamp "
      "bar.bundle/Contents/Info.plist "
      "bar.bundle/Contents/Resources/input1.txt "
      "bar.bundle/Contents/Resources/input2.txt "
      "bar.bundle/Contents/Resources/Assets.car "
      "baz/bar/bar_partial_info.plist || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

// Tests code signing steps.
TEST(NinjaCreateBundleTargetWriter, CodeSigning) {
  Err err;
  TestWithScope setup;

  std::unique_ptr<Target> action = NewAction(setup);
  ASSERT_TRUE(action->OnResolved(&err)) << err.message();

  Target executable(setup.settings(), Label(SourceDir("//baz/"), "quz"));
  executable.set_output_type(Target::EXECUTABLE);
  executable.sources().push_back(SourceFile("//baz/quz.c"));
  executable.SetToolchain(setup.toolchain());
  executable.visibility().SetPublic();
  ASSERT_TRUE(executable.OnResolved(&err));

  Target bundle_data(setup.settings(), Label(SourceDir("//foo/"), "data"));
  bundle_data.set_output_type(Target::BUNDLE_DATA);
  bundle_data.sources().push_back(SourceFile("//foo/input1.txt"));
  bundle_data.sources().push_back(SourceFile("//foo/input2.txt"));
  bundle_data.action_values().outputs() = SubstitutionList::MakeForTest(
      "{{bundle_resources_dir}}/{{source_file_part}}");
  bundle_data.SetToolchain(setup.toolchain());
  bundle_data.visibility().SetPublic();
  ASSERT_TRUE(bundle_data.OnResolved(&err));

  Target create_bundle(
      setup.settings(),
      Label(SourceDir("//baz/"), "bar", setup.toolchain()->label().dir(),
            setup.toolchain()->label().name()));
  SetupBundleDataDir(&create_bundle.bundle_data(), "//out/Debug");
  create_bundle.set_output_type(Target::CREATE_BUNDLE);
  create_bundle.bundle_data().set_code_signing_script(
      SourceFile("//build/codesign.py"));
  create_bundle.bundle_data().code_signing_sources().push_back(
      SourceFile("//out/Debug/quz"));
  create_bundle.bundle_data().code_signing_outputs() =
      SubstitutionList::MakeForTest(
          "//out/Debug/bar.bundle/Contents/quz",
          "//out/Debug/bar.bundle/_CodeSignature/CodeResources");
  create_bundle.bundle_data().code_signing_args() =
      SubstitutionList::MakeForTest("-b=quz", "bar.bundle");
  create_bundle.public_deps().push_back(LabelTargetPair(&executable));
  create_bundle.private_deps().push_back(LabelTargetPair(&bundle_data));
  create_bundle.private_deps().push_back(LabelTargetPair(action.get()));
  create_bundle.SetToolchain(setup.toolchain());
  ASSERT_TRUE(create_bundle.OnResolved(&err));

  std::ostringstream out;
  NinjaCreateBundleTargetWriter writer(&create_bundle, out);
  writer.Run();

  const char expected[] =
      "build obj/baz/bar.inputdeps.stamp: stamp ./quz obj/foo/bar.stamp "
      "obj/foo/data.stamp\n"
      "rule __baz_bar___toolchain_default__code_signing_rule\n"
      "  command =  ../../build/codesign.py -b=quz bar.bundle\n"
      "  description = CODE SIGNING //baz:bar(//toolchain:default)\n"
      "  restat = 1\n"
      "\n"
      "build bar.bundle/Contents/Resources/input1.txt: copy_bundle_data "
      "../../foo/input1.txt || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle/Contents/Resources/input2.txt: copy_bundle_data "
      "../../foo/input2.txt || obj/baz/bar.inputdeps.stamp\n"
      "build obj/baz/bar.codesigning.inputdeps.stamp: stamp "
      "../../build/codesign.py "
      "quz "
      "bar.bundle/Contents/Resources/input1.txt "
      "bar.bundle/Contents/Resources/input2.txt || "
      "obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle/Contents/quz bar.bundle/_CodeSignature/CodeResources: "
      "__baz_bar___toolchain_default__code_signing_rule "
      "| obj/baz/bar.codesigning.inputdeps.stamp\n"
      "build obj/baz/bar.stamp: stamp "
      "bar.bundle/Contents/quz "
      "bar.bundle/_CodeSignature/CodeResources || obj/baz/bar.inputdeps.stamp\n"
      "build bar.bundle: phony obj/baz/bar.stamp\n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}
