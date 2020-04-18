// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

using ActionTargetGenerator = TestWithScheduler;

// Tests that actions can't have output substitutions.
TEST_F(ActionTargetGenerator, ActionOutputSubstitutions) {
  TestWithScope setup;
  Scope::ItemVector items_;
  setup.scope()->set_item_collector(&items_);

  // First test one with no substitutions, this should be valid.
  TestParseInput input_good(
      R"(action("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/one.txt" ]
         })");
  ASSERT_FALSE(input_good.has_error());

  // This should run fine.
  Err err;
  input_good.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  // Same thing with a pattern in the output should fail.
  TestParseInput input_bad(
      R"(action("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}.txt" ]
         })");
  ASSERT_FALSE(input_bad.has_error());

  // This should run fine.
  input_bad.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
}

// Tests that arg and response file substitutions are validated for
// action_foreach targets.
TEST_F(ActionTargetGenerator, ActionForeachSubstitutions) {
  TestWithScope setup;
  Scope::ItemVector items_;
  setup.scope()->set_item_collector(&items_);

  // Args listing a response file but missing a response file definition should
  // fail.
  TestParseInput input_missing_resp_file(
      R"(action_foreach("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}" ]
           args = [ "{{response_file_name}}" ]
         })");
  ASSERT_FALSE(input_missing_resp_file.has_error());
  Err err;
  input_missing_resp_file.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());

  // Adding a response file definition should pass.
  err = Err();
  TestParseInput input_resp_file(
      R"(action_foreach("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}" ]
           args = [ "{{response_file_name}}" ]
           response_file_contents = [ "{{source_name_part}}" ]
         })");
  ASSERT_FALSE(input_resp_file.has_error());
  input_resp_file.parsed()->Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error()) << err.message();

  // Defining a response file but not referencing it should fail.
  err = Err();
  TestParseInput input_missing_rsp_args(
      R"(action_foreach("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}" ]
           args = [ "{{source_name_part}}" ]
           response_file_contents = [ "{{source_name_part}}" ]
         })");
  ASSERT_FALSE(input_missing_rsp_args.has_error());
  input_missing_rsp_args.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error()) << err.message();

  // Bad substitutions in args.
  err = Err();
  TestParseInput input_bad_args(
      R"(action_foreach("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}" ]
           args = [ "{{response_file_name}} {{ldflags}}" ]
           response_file_contents = [ "{{source_name_part}}" ]
         })");
  ASSERT_FALSE(input_bad_args.has_error());
  input_bad_args.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error()) << err.message();

  // Bad substitutions in response file contents.
  err = Err();
  TestParseInput input_bad_rsp(
      R"(action_foreach("foo") {
           script = "//foo.py"
           sources = [ "//bar.txt" ]
           outputs = [ "//out/Debug/{{source_name_part}}" ]
           args = [ "{{response_file_name}}" ]
           response_file_contents = [ "{{source_name_part}} {{ldflags}}" ]
         })");
  ASSERT_FALSE(input_bad_rsp.has_error());
  input_bad_rsp.parsed()->Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error()) << err.message();
}
