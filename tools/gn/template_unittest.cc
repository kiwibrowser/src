// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/test_with_scope.h"

TEST(Template, Basic) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  print(invoker.bar)\n"
      "}\n"
      "foo(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  EXPECT_EQ("lala\n42\n", setup.print_output());
}

TEST(Template, UnusedTargetNameShouldThrowError) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(invoker.bar)\n"
      "}\n"
      "foo(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
}

TEST(Template, UnusedInvokerShouldThrowError) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "}\n"
      "foo(\"lala\") {\n"
      "  bar = 42\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
}

TEST(Template, UnusedVarInInvokerShouldThrowError) {
  TestWithScope setup;
  TestParseInput input(
      "template(\"foo\") {\n"
      "  print(target_name)\n"
      "  print(invoker.bar)\n"
      "}\n"
      "foo(\"lala\") {\n"
      "  bar = 42\n"
      "  baz = [ \"foo\" ]\n"
      "}");
  ASSERT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
}

// Previous versions of the template implementation would copy templates by
// value when it makes a closure. Doing a sequence of them means that every new
// one copies all previous ones, which gives a significant blow-up in memory.
// If this test doesn't crash with out-of-memory, it passed.
TEST(Template, MemoryBlowUp) {
  TestWithScope setup;
  std::string code;
  for (int i = 0; i < 100; i++)
    code += "template(\"test" + base::IntToString(i) + "\") {}\n";

  TestParseInput input(code);

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(input.has_error());
}
