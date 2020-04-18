// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/ninja_binary_target_writer.h"

#include <memory>
#include <sstream>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/config.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

using NinjaBinaryTargetWriterTest = TestWithScheduler;

TEST_F(NinjaBinaryTargetWriterTest, SourceSet) {
  Err err;
  TestWithScope setup;

  Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  target.set_output_type(Target::SOURCE_SET);
  target.visibility().SetPublic();
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  // Also test object files, which should be just passed through to the
  // dependents to link.
  target.sources().push_back(SourceFile("//foo/input3.o"));
  target.sources().push_back(SourceFile("//foo/input4.obj"));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  // Source set itself.
  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj\n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }

  // A shared library that depends on the source set.
  Target shlib_target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  shlib_target.set_output_type(Target::SHARED_LIBRARY);
  shlib_target.public_deps().push_back(LabelTargetPair(&target));
  shlib_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(shlib_target.OnResolved(&err));

  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&shlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libshlib\n"
        "\n"
        "\n"
        // Ordering of the obj files here should come out in the order
        // specified, with the target's first, followed by the source set's, in
        // order.
        "build ./libshlib.so: solink obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj "
            "|| obj/foo/bar.stamp\n"
        "  ldflags =\n"
        "  libs =\n"
        "  output_extension = .so\n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }

  // A static library that depends on the source set (should not link it).
  Target stlib_target(setup.settings(), Label(SourceDir("//foo/"), "stlib"));
  stlib_target.set_output_type(Target::STATIC_LIBRARY);
  stlib_target.public_deps().push_back(LabelTargetPair(&target));
  stlib_target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(stlib_target.OnResolved(&err));

  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&stlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libstlib\n"
        "\n"
        "\n"
        // There are no sources so there are no params to alink. (In practice
        // this will probably fail in the archive tool.)
        "build obj/foo/libstlib.a: alink || obj/foo/bar.stamp\n"
        "  arflags =\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }

  // Make the static library 'complete', which means it should be linked.
  stlib_target.set_complete_static_lib(true);
  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&stlib_target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libstlib\n"
        "\n"
        "\n"
        // Ordering of the obj files here should come out in the order
        // specified, with the target's first, followed by the source set's, in
        // order.
        "build obj/foo/libstlib.a: alink obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o ../../foo/input3.o ../../foo/input4.obj "
            "|| obj/foo/bar.stamp\n"
        "  arflags =\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }
}

TEST_F(NinjaBinaryTargetWriterTest, StaticLibrary) {
  TestWithScope setup;
  Err err;

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.config_values().arflags().push_back("--asdf");
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
      "\n"
      "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o\n"
      "  arflags = --asdf\n"
      "  output_extension = \n"
      "  output_dir = \n";
  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, CompleteStaticLibrary) {
  TestWithScope setup;
  Err err;

  TestTarget target(setup, "//foo:bar", Target::STATIC_LIBRARY);
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.config_values().arflags().push_back("--asdf");
  target.set_complete_static_lib(true);

  TestTarget baz(setup, "//foo:baz", Target::STATIC_LIBRARY);
  baz.sources().push_back(SourceFile("//foo/input2.cc"));

  target.public_deps().push_back(LabelTargetPair(&baz));

  ASSERT_TRUE(target.OnResolved(&err));
  ASSERT_TRUE(baz.OnResolved(&err));

  // A complete static library that depends on an incomplete static library
  // should link in the dependent object files as if the dependent target
  // were a source set.
  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
        "\n"
        "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o "
            "obj/foo/libbaz.input2.o || obj/foo/libbaz.a\n"
        "  arflags = --asdf\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }

  // Make the dependent static library complete.
  baz.set_complete_static_lib(true);

  // Dependent complete static libraries should not be linked directly.
  {
    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "build obj/foo/libbar.input1.o: cxx ../../foo/input1.cc\n"
        "\n"
        "build obj/foo/libbar.a: alink obj/foo/libbar.input1.o "
            "|| obj/foo/libbaz.a\n"
        "  arflags = --asdf\n"
        "  output_extension = \n"
        "  output_dir = \n";
    std::string out_str = out.str();
    EXPECT_EQ(expected, out_str);
  }
}

// This tests that output extension and output dir overrides apply, and input
// dependencies are applied.
TEST_F(NinjaBinaryTargetWriterTest, OutputExtensionAndInputDeps) {
  Err err;
  TestWithScope setup;

  // An action for our library to depend on.
  Target action(setup.settings(), Label(SourceDir("//foo/"), "action"));
  action.set_output_type(Target::ACTION_FOREACH);
  action.visibility().SetPublic();
  action.SetToolchain(setup.toolchain());
  ASSERT_TRUE(action.OnResolved(&err));

  // A shared library w/ the output_extension set to a custom value.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_extension(std::string("so.6"));
  target.set_output_dir(SourceDir("//out/Debug/foo/"));
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));
  target.public_deps().push_back(LabelTargetPair(&action));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libshlib\n"
      "\n"
      "build obj/foo/libshlib.input1.o: cxx ../../foo/input1.cc"
        " || obj/foo/action.stamp\n"
      "build obj/foo/libshlib.input2.o: cxx ../../foo/input2.cc"
        " || obj/foo/action.stamp\n"
      "\n"
      "build ./libshlib.so.6: solink obj/foo/libshlib.input1.o "
      // The order-only dependency here is stricly unnecessary since the
      // sources list this as an order-only dep. See discussion in the code
      // that writes this.
          "obj/foo/libshlib.input2.o || obj/foo/action.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = .so.6\n"
      "  output_dir = foo\n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, NoHardDepsToNoPublicHeaderTarget) {
  Err err;
  TestWithScope setup;

  SourceFile generated_file("//out/Debug/generated.cc");

  // An action does code generation.
  Target action(setup.settings(), Label(SourceDir("//foo/"), "generate"));
  action.set_output_type(Target::ACTION);
  action.visibility().SetPublic();
  action.SetToolchain(setup.toolchain());
  action.set_output_dir(SourceDir("//out/Debug/foo/"));
  action.action_values().outputs() =
      SubstitutionList::MakeForTest("//out/Debug/generated.cc");
  ASSERT_TRUE(action.OnResolved(&err));

  // A source set compiling geneated code, this target does not publicize any
  // headers.
  Target gen_obj(setup.settings(), Label(SourceDir("//foo/"), "gen_obj"));
  gen_obj.set_output_type(Target::SOURCE_SET);
  gen_obj.set_output_dir(SourceDir("//out/Debug/foo/"));
  gen_obj.sources().push_back(generated_file);
  gen_obj.visibility().SetPublic();
  gen_obj.private_deps().push_back(LabelTargetPair(&action));
  gen_obj.set_all_headers_public(false);
  gen_obj.SetToolchain(setup.toolchain());
  ASSERT_TRUE(gen_obj.OnResolved(&err));

  std::ostringstream obj_out;
  NinjaBinaryTargetWriter obj_writer(&gen_obj, obj_out);
  obj_writer.Run();

  const char obj_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = gen_obj\n"
      "\n"
      "build obj/out/Debug/gen_obj.generated.o: cxx generated.cc"
      " || obj/foo/generate.stamp\n"
      "\n"
      "build obj/foo/gen_obj.stamp: stamp obj/out/Debug/gen_obj.generated.o"
      // The order-only dependency here is strictly unnecessary since the
      // sources list this as an order-only dep.
      " || obj/foo/generate.stamp\n";

  std::string obj_str = obj_out.str();
  EXPECT_EQ(obj_expected, obj_str);

  // A shared library depends on gen_obj, having corresponding header for
  // generated obj.
  Target gen_lib(setup.settings(), Label(SourceDir("//foo/"), "gen_lib"));
  gen_lib.set_output_type(Target::SHARED_LIBRARY);
  gen_lib.set_output_dir(SourceDir("//out/Debug/foo/"));
  gen_lib.sources().push_back(SourceFile("//foor/generated.h"));
  gen_lib.visibility().SetPublic();
  gen_lib.private_deps().push_back(LabelTargetPair(&gen_obj));
  gen_lib.SetToolchain(setup.toolchain());
  ASSERT_TRUE(gen_lib.OnResolved(&err));

  std::ostringstream lib_out;
  NinjaBinaryTargetWriter lib_writer(&gen_lib, lib_out);
  lib_writer.Run();

  const char lib_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libgen_lib\n"
      "\n"
      "\n"
      "build ./libgen_lib.so: solink obj/out/Debug/gen_obj.generated.o"
      // The order-only dependency here is strictly unnecessary since
      // obj/out/Debug/gen_obj.generated.o has dependency to
      // obj/foo/gen_obj.stamp
      " || obj/foo/gen_obj.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = .so\n"
      "  output_dir = foo\n";

  std::string lib_str = lib_out.str();
  EXPECT_EQ(lib_expected, lib_str);

  // An executable depends on gen_lib.
  Target executable(setup.settings(),
                    Label(SourceDir("//foo/"), "final_target"));
  executable.set_output_type(Target::EXECUTABLE);
  executable.set_output_dir(SourceDir("//out/Debug/foo/"));
  executable.sources().push_back(SourceFile("//foo/main.cc"));
  executable.private_deps().push_back(LabelTargetPair(&gen_lib));
  executable.SetToolchain(setup.toolchain());
  ASSERT_TRUE(executable.OnResolved(&err)) << err.message();

  std::ostringstream final_out;
  NinjaBinaryTargetWriter final_writer(&executable, final_out);
  final_writer.Run();

  // There is no order only dependency to action target.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = final_target\n"
      "\n"
      "build obj/foo/final_target.main.o: cxx ../../foo/main.cc\n"
      "\n"
      "build ./final_target: link obj/foo/final_target.main.o"
      " ./libgen_lib.so\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = \n"
      "  output_dir = foo\n";

  std::string final_str = final_out.str();
  EXPECT_EQ(final_expected, final_str);
}

// Tests libs are applied.
TEST_F(NinjaBinaryTargetWriterTest, LibsAndLibDirs) {
  Err err;
  TestWithScope setup;

  // A shared library w/ libs and lib_dirs.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.config_values().libs().push_back(LibFile(SourceFile("//foo/lib1.a")));
  target.config_values().libs().push_back(LibFile("foo"));
  target.config_values().lib_dirs().push_back(SourceDir("//foo/bar/"));
  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libshlib\n"
      "\n"
      "\n"
      "build ./libshlib.so: solink | ../../foo/lib1.a\n"
      "  ldflags = -L../../foo/bar\n"
      "  libs = ../../foo/lib1.a -lfoo\n"
      "  output_extension = .so\n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, EmptyOutputExtension) {
  Err err;
  TestWithScope setup;

  // This test is the same as OutputExtensionAndInputDeps, except that we call
  // set_output_extension("") and ensure that we get an empty one and override
  // the output prefix so that the name matches the target exactly.
  Target target(setup.settings(), Label(SourceDir("//foo/"), "shlib"));
  target.set_output_type(Target::SHARED_LIBRARY);
  target.set_output_prefix_override(true);
  target.set_output_extension(std::string());
  target.sources().push_back(SourceFile("//foo/input1.cc"));
  target.sources().push_back(SourceFile("//foo/input2.cc"));

  target.SetToolchain(setup.toolchain());
  ASSERT_TRUE(target.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = shlib\n"
      "\n"
      "build obj/foo/shlib.input1.o: cxx ../../foo/input1.cc\n"
      "build obj/foo/shlib.input2.o: cxx ../../foo/input2.cc\n"
      "\n"
      "build ./shlib: solink obj/foo/shlib.input1.o "
          "obj/foo/shlib.input2.o\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = \n"
      "  output_dir = \n";

  std::string out_str = out.str();
  EXPECT_EQ(expected, out_str);
}

TEST_F(NinjaBinaryTargetWriterTest, SourceSetDataDeps) {
  Err err;
  TestWithScope setup;

  // This target is a data (runtime) dependency of the intermediate target.
  Target data(setup.settings(), Label(SourceDir("//foo/"), "data_target"));
  data.set_output_type(Target::EXECUTABLE);
  data.visibility().SetPublic();
  data.SetToolchain(setup.toolchain());
  ASSERT_TRUE(data.OnResolved(&err));

  // Intermediate source set target.
  Target inter(setup.settings(), Label(SourceDir("//foo/"), "inter"));
  inter.set_output_type(Target::SOURCE_SET);
  inter.visibility().SetPublic();
  inter.data_deps().push_back(LabelTargetPair(&data));
  inter.SetToolchain(setup.toolchain());
  inter.sources().push_back(SourceFile("//foo/inter.cc"));
  ASSERT_TRUE(inter.OnResolved(&err)) << err.message();

  // Write out the intermediate target.
  std::ostringstream inter_out;
  NinjaBinaryTargetWriter inter_writer(&inter, inter_out);
  inter_writer.Run();

  // The intermediate source set will be a stamp file that depends on the
  // object files, and will have an order-only dependency on its data dep and
  // data file.
  const char inter_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = inter\n"
      "\n"
      "build obj/foo/inter.inter.o: cxx ../../foo/inter.cc\n"
      "\n"
      "build obj/foo/inter.stamp: stamp obj/foo/inter.inter.o || "
          "./data_target\n";
  EXPECT_EQ(inter_expected, inter_out.str());

  // Final target.
  Target exe(setup.settings(), Label(SourceDir("//foo/"), "exe"));
  exe.set_output_type(Target::EXECUTABLE);
  exe.public_deps().push_back(LabelTargetPair(&inter));
  exe.SetToolchain(setup.toolchain());
  exe.sources().push_back(SourceFile("//foo/final.cc"));
  ASSERT_TRUE(exe.OnResolved(&err));

  std::ostringstream final_out;
  NinjaBinaryTargetWriter final_writer(&exe, final_out);
  final_writer.Run();

  // The final output depends on both object files (one from the final target,
  // one from the source set) and has an order-only dependency on the source
  // set's stamp file and the final target's data file. The source set stamp
  // dependency will create an implicit order-only dependency on the data
  // target.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/foo/exe.final.o: cxx ../../foo/final.cc\n"
      "\n"
      "build ./exe: link obj/foo/exe.final.o obj/foo/inter.inter.o || "
          "obj/foo/inter.stamp\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  EXPECT_EQ(final_expected, final_out.str());
}

TEST_F(NinjaBinaryTargetWriterTest, SharedLibraryModuleDefinitionFile) {
  Err err;
  TestWithScope setup;

  Target shared_lib(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  shared_lib.set_output_type(Target::SHARED_LIBRARY);
  shared_lib.SetToolchain(setup.toolchain());
  shared_lib.sources().push_back(SourceFile("//foo/sources.cc"));
  shared_lib.sources().push_back(SourceFile("//foo/bar.def"));
  ASSERT_TRUE(shared_lib.OnResolved(&err));

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&shared_lib, out);
  writer.Run();

  const char expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.sources.o: cxx ../../foo/sources.cc\n"
      "\n"
      "build ./libbar.so: solink obj/foo/libbar.sources.o | ../../foo/bar.def\n"
      "  ldflags = /DEF:../../foo/bar.def\n"
      "  libs =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";
  EXPECT_EQ(expected, out.str());
}

TEST_F(NinjaBinaryTargetWriterTest, LoadableModule) {
  Err err;
  TestWithScope setup;

  Target loadable_module(setup.settings(), Label(SourceDir("//foo/"), "bar"));
  loadable_module.set_output_type(Target::LOADABLE_MODULE);
  loadable_module.visibility().SetPublic();
  loadable_module.SetToolchain(setup.toolchain());
  loadable_module.sources().push_back(SourceFile("//foo/sources.cc"));
  ASSERT_TRUE(loadable_module.OnResolved(&err)) << err.message();

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&loadable_module, out);
  writer.Run();

  const char loadable_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = libbar\n"
      "\n"
      "build obj/foo/libbar.sources.o: cxx ../../foo/sources.cc\n"
      "\n"
      "build ./libbar.so: solink_module obj/foo/libbar.sources.o\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = .so\n"
      "  output_dir = \n";
  EXPECT_EQ(loadable_expected, out.str());

  // Final target.
  Target exe(setup.settings(), Label(SourceDir("//foo/"), "exe"));
  exe.set_output_type(Target::EXECUTABLE);
  exe.public_deps().push_back(LabelTargetPair(&loadable_module));
  exe.SetToolchain(setup.toolchain());
  exe.sources().push_back(SourceFile("//foo/final.cc"));
  ASSERT_TRUE(exe.OnResolved(&err)) << err.message();

  std::ostringstream final_out;
  NinjaBinaryTargetWriter final_writer(&exe, final_out);
  final_writer.Run();

  // The final output depends on the loadable module so should have an
  // order-only dependency on the loadable modules's output file.
  const char final_expected[] =
      "defines =\n"
      "include_dirs =\n"
      "cflags =\n"
      "cflags_cc =\n"
      "root_out_dir = .\n"
      "target_out_dir = obj/foo\n"
      "target_output_name = exe\n"
      "\n"
      "build obj/foo/exe.final.o: cxx ../../foo/final.cc\n"
      "\n"
      "build ./exe: link obj/foo/exe.final.o || ./libbar.so\n"
      "  ldflags =\n"
      "  libs =\n"
      "  output_extension = \n"
      "  output_dir = \n";
  EXPECT_EQ(final_expected, final_out.str());
}

TEST_F(NinjaBinaryTargetWriterTest, WinPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  TestWithScope setup;

  // A precompiled header toolchain.
  Settings pch_settings(setup.build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(setup.toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx_tool = std::make_unique<Tool>();
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cxx_tool.get());
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_precompiled_header_type(Tool::PCH_MSVC);
  pch_toolchain.SetTool(Toolchain::TYPE_CXX, std::move(cxx_tool));

  // Add a C compiler as well.
  std::unique_ptr<Tool> cc_tool = std::make_unique<Tool>();
  TestWithScope::SetCommandForTool(
      "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cc_tool.get());
  cc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cc_tool->set_precompiled_header_type(Tool::PCH_MSVC);
  pch_toolchain.SetTool(Toolchain::TYPE_CC, std::move(cc_tool));
  pch_toolchain.ToolchainSetupComplete();

  // This target doesn't specify precompiled headers.
  {
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&no_pch_target, out);
    writer.Run();

    const char no_pch_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99\n"
        "cflags_cc =\n"
        "target_output_name = no_pch_target\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.input1.o: "
               "withpch_cxx ../../foo/input1.cc\n"
        "build withpch/obj/foo/no_pch_target.input2.o: "
               "withpch_cc ../../foo/input2.c\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.stamp: "
               "withpch_stamp withpch/obj/foo/no_pch_target.input1.o "
               "withpch/obj/foo/no_pch_target.input2.o\n";
    EXPECT_EQ(no_pch_expected, out.str());
  }

  // This target specifies PCH.
  {
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_header("build/precompile.h");
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.cc"));
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&pch_target, out);
    writer.Run();

    const char pch_win_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        // It should output language-specific pch files.
        "cflags_c = /Fpwithpch/obj/foo/pch_target_c.pch "
                    "/Yubuild/precompile.h\n"
        "cflags_cc = /Fpwithpch/obj/foo/pch_target_cc.pch "
                     "/Yubuild/precompile.h\n"
        "target_output_name = pch_target\n"
        "\n"
        // Compile the precompiled source files with /Yc.
        "build withpch/obj/build/pch_target.precompile.c.o: "
               "withpch_cc ../../build/precompile.cc\n"
        "  cflags_c = ${cflags_c} /Ycbuild/precompile.h\n"
        "\n"
        "build withpch/obj/build/pch_target.precompile.cc.o: "
               "withpch_cxx ../../build/precompile.cc\n"
        "  cflags_cc = ${cflags_cc} /Ycbuild/precompile.h\n"
        "\n"
        "build withpch/obj/foo/pch_target.input1.o: "
               "withpch_cxx ../../foo/input1.cc | "
               // Explicit dependency on the PCH build step.
               "withpch/obj/build/pch_target.precompile.cc.o\n"
        "build withpch/obj/foo/pch_target.input2.o: "
               "withpch_cc ../../foo/input2.c | "
               // Explicit dependency on the PCH build step.
               "withpch/obj/build/pch_target.precompile.c.o\n"
        "\n"
        "build withpch/obj/foo/pch_target.stamp: withpch_stamp "
               "withpch/obj/foo/pch_target.input1.o "
               "withpch/obj/foo/pch_target.input2.o "
               // The precompiled object files were added to the outputs.
               "withpch/obj/build/pch_target.precompile.c.o "
               "withpch/obj/build/pch_target.precompile.cc.o\n";
    EXPECT_EQ(pch_win_expected, out.str());
  }
}

TEST_F(NinjaBinaryTargetWriterTest, GCCPrecompiledHeaders) {
  Err err;

  // This setup's toolchain does not have precompiled headers defined.
  TestWithScope setup;

  // A precompiled header toolchain.
  Settings pch_settings(setup.build_settings(), "withpch/");
  Toolchain pch_toolchain(&pch_settings,
                          Label(SourceDir("//toolchain/"), "withpch"));
  pch_settings.set_toolchain_label(pch_toolchain.label());
  pch_settings.set_default_toolchain_label(setup.toolchain()->label());

  // Declare a C++ compiler that supports PCH.
  std::unique_ptr<Tool> cxx_tool = std::make_unique<Tool>();
  TestWithScope::SetCommandForTool(
      "c++ {{source}} {{cflags}} {{cflags_cc}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cxx_tool.get());
  cxx_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cxx_tool->set_precompiled_header_type(Tool::PCH_GCC);
  pch_toolchain.SetTool(Toolchain::TYPE_CXX, std::move(cxx_tool));
  pch_toolchain.ToolchainSetupComplete();

  // Add a C compiler as well.
  std::unique_ptr<Tool> cc_tool = std::make_unique<Tool>();
  TestWithScope::SetCommandForTool(
      "cc {{source}} {{cflags}} {{cflags_c}} {{defines}} {{include_dirs}} "
      "-o {{output}}",
      cc_tool.get());
  cc_tool->set_outputs(SubstitutionList::MakeForTest(
      "{{source_out_dir}}/{{target_output_name}}.{{source_name_part}}.o"));
  cc_tool->set_precompiled_header_type(Tool::PCH_GCC);
  pch_toolchain.SetTool(Toolchain::TYPE_CC, std::move(cc_tool));
  pch_toolchain.ToolchainSetupComplete();

  // This target doesn't specify precompiled headers.
  {
    Target no_pch_target(&pch_settings,
                         Label(SourceDir("//foo/"), "no_pch_target"));
    no_pch_target.set_output_type(Target::SOURCE_SET);
    no_pch_target.visibility().SetPublic();
    no_pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    no_pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    no_pch_target.config_values().cflags_c().push_back("-std=c99");
    no_pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(no_pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&no_pch_target, out);
    writer.Run();

    const char no_pch_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99\n"
        "cflags_cc =\n"
        "target_output_name = no_pch_target\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.input1.o: "
               "withpch_cxx ../../foo/input1.cc\n"
        "build withpch/obj/foo/no_pch_target.input2.o: "
               "withpch_cc ../../foo/input2.c\n"
        "\n"
        "build withpch/obj/foo/no_pch_target.stamp: "
               "withpch_stamp withpch/obj/foo/no_pch_target.input1.o "
               "withpch/obj/foo/no_pch_target.input2.o\n";
    EXPECT_EQ(no_pch_expected, out.str());
  }

  // This target specifies PCH.
  {
    Target pch_target(&pch_settings, Label(SourceDir("//foo/"), "pch_target"));
    pch_target.config_values().set_precompiled_source(
        SourceFile("//build/precompile.h"));
    pch_target.config_values().cflags_c().push_back("-std=c99");
    pch_target.set_output_type(Target::SOURCE_SET);
    pch_target.visibility().SetPublic();
    pch_target.sources().push_back(SourceFile("//foo/input1.cc"));
    pch_target.sources().push_back(SourceFile("//foo/input2.c"));
    pch_target.SetToolchain(&pch_toolchain);
    ASSERT_TRUE(pch_target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&pch_target, out);
    writer.Run();

    const char pch_gcc_expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_c = -std=c99 "
                    "-include withpch/obj/build/pch_target.precompile.h-c\n"
        "cflags_cc = -include withpch/obj/build/pch_target.precompile.h-cc\n"
        "target_output_name = pch_target\n"
        "\n"
        // Compile the precompiled sources with -x <lang>.
        "build withpch/obj/build/pch_target.precompile.h-c.gch: "
               "withpch_cc ../../build/precompile.h\n"
        "  cflags_c = -std=c99 -x c-header\n"
        "\n"
        "build withpch/obj/build/pch_target.precompile.h-cc.gch: "
               "withpch_cxx ../../build/precompile.h\n"
        "  cflags_cc = -x c++-header\n"
        "\n"
        "build withpch/obj/foo/pch_target.input1.o: "
               "withpch_cxx ../../foo/input1.cc | "
               // Explicit dependency on the PCH build step.
               "withpch/obj/build/pch_target.precompile.h-cc.gch\n"
        "build withpch/obj/foo/pch_target.input2.o: "
               "withpch_cc ../../foo/input2.c | "
               // Explicit dependency on the PCH build step.
               "withpch/obj/build/pch_target.precompile.h-c.gch\n"
        "\n"
        "build withpch/obj/foo/pch_target.stamp: "
               "withpch_stamp withpch/obj/foo/pch_target.input1.o "
               "withpch/obj/foo/pch_target.input2.o\n";
    EXPECT_EQ(pch_gcc_expected, out.str());
  }
}

// Should throw an error with the scheduler if a duplicate object file exists.
// This is dependent on the toolchain's object file mapping.
TEST_F(NinjaBinaryTargetWriterTest, DupeObjFileError) {
  TestWithScope setup;
  TestTarget target(setup, "//foo:bar", Target::EXECUTABLE);
  target.sources().push_back(SourceFile("//a.cc"));
  target.sources().push_back(SourceFile("//a.cc"));

  EXPECT_FALSE(scheduler().is_failed());

  std::ostringstream out;
  NinjaBinaryTargetWriter writer(&target, out);
  writer.Run();

  // Should have issued an error.
  EXPECT_TRUE(scheduler().is_failed());
}

// This tests that output extension and output dir overrides apply, and input
// dependencies are applied.
TEST_F(NinjaBinaryTargetWriterTest, InputFiles) {
  Err err;
  TestWithScope setup;

  // This target has one input.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.config_values().inputs().push_back(SourceFile("//foo/input.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
          " | ../../foo/input.data\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
          " | ../../foo/input.data\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has one input but no source files.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SHARED_LIBRARY);
    target.visibility().SetPublic();
    target.config_values().inputs().push_back(SourceFile("//foo/input.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = libbar\n"
        "\n"
        "\n"
        "build ./libbar.so: solink | ../../foo/input.data\n"
        "  ldflags =\n"
        "  libs =\n"
        "  output_extension = .so\n"
        "  output_dir = \n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has multiple inputs.
  {
    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.config_values().inputs().push_back(SourceFile("//foo/input1.data"));
    target.config_values().inputs().push_back(SourceFile("//foo/input2.data"));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.inputs.stamp: stamp"
          " ../../foo/input1.data ../../foo/input2.data\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
          " | obj/foo/bar.inputs.stamp\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
          " | obj/foo/bar.inputs.stamp\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }

  // This target has one input itself, one from an immediate config, and one
  // from a config tacked on to said config.
  {
    Config far_config(setup.settings(), Label(SourceDir("//foo/"), "qux"));
    far_config.own_values().inputs().push_back(SourceFile("//foo/input3.data"));
    ASSERT_TRUE(far_config.OnResolved(&err));

    Config config(setup.settings(), Label(SourceDir("//foo/"), "baz"));
    config.own_values().inputs().push_back(SourceFile("//foo/input2.data"));
    config.configs().push_back(LabelConfigPair(&far_config));
    ASSERT_TRUE(config.OnResolved(&err));

    Target target(setup.settings(), Label(SourceDir("//foo/"), "bar"));
    target.set_output_type(Target::SOURCE_SET);
    target.visibility().SetPublic();
    target.sources().push_back(SourceFile("//foo/input1.cc"));
    target.sources().push_back(SourceFile("//foo/input2.cc"));
    target.config_values().inputs().push_back(SourceFile("//foo/input1.data"));
    target.configs().push_back(LabelConfigPair(&config));
    target.SetToolchain(setup.toolchain());
    ASSERT_TRUE(target.OnResolved(&err));

    std::ostringstream out;
    NinjaBinaryTargetWriter writer(&target, out);
    writer.Run();

    const char expected[] =
        "defines =\n"
        "include_dirs =\n"
        "cflags =\n"
        "cflags_cc =\n"
        "root_out_dir = .\n"
        "target_out_dir = obj/foo\n"
        "target_output_name = bar\n"
        "\n"
        "build obj/foo/bar.inputs.stamp: stamp"
          " ../../foo/input1.data ../../foo/input2.data ../../foo/input3.data\n"
        "build obj/foo/bar.input1.o: cxx ../../foo/input1.cc"
          " | obj/foo/bar.inputs.stamp\n"
        "build obj/foo/bar.input2.o: cxx ../../foo/input2.cc"
          " | obj/foo/bar.inputs.stamp\n"
        "\n"
        "build obj/foo/bar.stamp: stamp obj/foo/bar.input1.o "
            "obj/foo/bar.input2.o\n";

    EXPECT_EQ(expected, out.str());
  }
}
