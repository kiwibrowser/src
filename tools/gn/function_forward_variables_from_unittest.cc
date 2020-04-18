// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

using FunctionForwardVariablesFromTest = TestWithScheduler;

TEST_F(FunctionForwardVariablesFromTest, List) {
  Err err;
  std::string program =
      "template(\"a\") {\n"
      "  forward_variables_from(invoker, [\"x\", \"y\", \"z\"])\n"
      "  assert(!defined(z))\n"  // "z" should still be undefined.
      "  print(\"$target_name, $x, $y\")\n"
      "}\n"
      "a(\"target\") {\n"
      "  x = 1\n"
      "  y = 2\n"
      "}\n";

  {
    TestWithScope setup;

    // Defines a template and copy the two x and y, and z values out.
    TestParseInput input(program);
    ASSERT_FALSE(input.has_error());

    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_FALSE(err.has_error()) << err.message();

    EXPECT_EQ("target, 1, 2\n", setup.print_output());
    setup.print_output().clear();
  }

  {
    TestWithScope setup;

    // Test that the same input but forwarding a variable with the name of
    // something in the given scope throws an error rather than clobbering it.
    // This uses the same known-good program as before, but adds another
    // variable in the scope before it.
    TestParseInput clobber("x = 1\n" + program);
    ASSERT_FALSE(clobber.has_error());

    clobber.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error());  // Should thow a clobber error.
    EXPECT_EQ("Clobbering existing value.", err.message());
  }
}

TEST_F(FunctionForwardVariablesFromTest, LiteralList) {
  TestWithScope setup;

  // Forwards all variables from a literal scope into another scope definition.
  TestParseInput input(
    "a = {\n"
    "  forward_variables_from({x = 1 y = 2}, \"*\")\n"
    "  z = 3\n"
    "}\n"
    "print(\"${a.x} ${a.y} ${a.z}\")\n");

  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ("1 2 3\n", setup.print_output());
  setup.print_output().clear();
}

TEST_F(FunctionForwardVariablesFromTest, ListWithExclusion) {
  TestWithScope setup;

  // Defines a template and copy the two x and y, and z values out.
  TestParseInput input(
    "template(\"a\") {\n"
    "  forward_variables_from(invoker, [\"x\", \"y\", \"z\"], [\"z\"])\n"
    "  assert(!defined(z))\n"  // "z" should still be undefined.
    "  print(\"$target_name, $x, $y\")\n"
    "}\n"
    "a(\"target\") {\n"
    "  x = 1\n"
    "  y = 2\n"
    "  z = 3\n"
    "  print(\"$z\")\n"
    "}\n");

  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ("3\ntarget, 1, 2\n", setup.print_output());
  setup.print_output().clear();
}

TEST_F(FunctionForwardVariablesFromTest, ErrorCases) {
  TestWithScope setup;

  // Type check the source scope.
  TestParseInput invalid_source(
    "template(\"a\") {\n"
    "  forward_variables_from(42, [\"x\"])\n"
    "  print(\"$target_name\")\n"  // Prevent unused var error.
    "}\n"
    "a(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(invalid_source.has_error());
  Err err;
  invalid_source.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("This is not a scope.", err.message());

  // Type check the list. We need to use a new template name each time since
  // all of these invocations are executing in sequence in the same scope.
  TestParseInput invalid_list(
    "template(\"b\") {\n"
    "  forward_variables_from(invoker, 42)\n"
    "  print(\"$target_name\")\n"
    "}\n"
    "b(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(invalid_list.has_error());
  err = Err();
  invalid_list.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Not a valid list of variables to copy.", err.message());

  // Type check the exclusion list.
  TestParseInput invalid_exclusion_list(
    "template(\"c\") {\n"
    "  forward_variables_from(invoker, \"*\", 42)\n"
    "  print(\"$target_name\")\n"
    "}\n"
    "c(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(invalid_exclusion_list.has_error());
  err = Err();
  invalid_exclusion_list.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Not a valid list of variables to exclude.", err.message());

  // Programmatic values should error.
  TestParseInput prog(
    "template(\"d\") {\n"
    "  forward_variables_from(invoker, [\"root_out_dir\"])\n"
    "  print(\"$target_name\")\n"
    "}\n"
    "d(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(prog.has_error());
  err = Err();
  prog.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("This value can't be forwarded.", err.message());

  // Not enough arguments.
  TestParseInput not_enough_arguments(
    "template(\"e\") {\n"
    "  forward_variables_from(invoker)\n"
    "  print(\"$target_name\")\n"
    "}\n"
    "e(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(not_enough_arguments.has_error());
  err = Err();
  not_enough_arguments.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Wrong number of arguments.", err.message());

  // Too many arguments.
  TestParseInput too_many_arguments(
    "template(\"f\") {\n"
    "  forward_variables_from(invoker, \"*\", [], [])\n"
    "  print(\"$target_name\")\n"
    "}\n"
    "f(\"target\") {\n"
    "}\n");
  ASSERT_FALSE(too_many_arguments.has_error());
  err = Err();
  too_many_arguments.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Wrong number of arguments.", err.message());
}

TEST_F(FunctionForwardVariablesFromTest, Star) {
  TestWithScope setup;

  // Defines a template and copy the two x and y values out. The "*" behavior
  // should clobber existing variables with the same name.
  TestParseInput input(
    "template(\"a\") {\n"
    "  x = 1000000\n"  // Should be clobbered.
    "  forward_variables_from(invoker, \"*\")\n"
    "  print(\"$target_name, $x, $y\")\n"
    "}\n"
    "a(\"target\") {\n"
    "  x = 1\n"
    "  y = 2\n"
    "}\n");

  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ("target, 1, 2\n", setup.print_output());
  setup.print_output().clear();
}

TEST_F(FunctionForwardVariablesFromTest, StarWithExclusion) {
  TestWithScope setup;

  // Defines a template and copy all values except z value. The "*" behavior
  // should clobber existing variables with the same name.
  TestParseInput input(
    "template(\"a\") {\n"
    "  x = 1000000\n"  // Should be clobbered.
    "  forward_variables_from(invoker, \"*\", [\"z\"])\n"
    "  print(\"$target_name, $x, $y\")\n"
    "}\n"
    "a(\"target\") {\n"
    "  x = 1\n"
    "  y = 2\n"
    "  z = 3\n"
    "  print(\"$z\")\n"
    "}\n");

  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ("3\ntarget, 1, 2\n", setup.print_output());
  setup.print_output().clear();
}
