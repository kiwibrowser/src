// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/functions.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

using FunctionToolchain = TestWithScheduler;

TEST_F(FunctionToolchain, RuntimeOutputs) {
  TestWithScope setup;

  // These runtime outputs are a subset of the outputs so are OK.
  {
    TestParseInput input(
        R"(toolchain("good") {
          tool("link") {
            outputs = [ "foo" ]
            runtime_outputs = [ "foo" ]
          }
        })");
    ASSERT_FALSE(input.has_error());

    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_FALSE(err.has_error()) << err.message();

    // It should have generated a toolchain.
    ASSERT_EQ(1u, setup.items().size());
    const Toolchain* toolchain = setup.items()[0]->AsToolchain();
    ASSERT_TRUE(toolchain);

    // The toolchain should have a link tool with the two outputs.
    const Tool* link = toolchain->GetTool(Toolchain::TYPE_LINK);
    ASSERT_TRUE(link);
    ASSERT_EQ(1u, link->outputs().list().size());
    EXPECT_EQ("foo", link->outputs().list()[0].AsString());
    ASSERT_EQ(1u, link->runtime_outputs().list().size());
    EXPECT_EQ("foo", link->runtime_outputs().list()[0].AsString());
  }

  // This one is not a subset so should throw an error.
  {
    TestParseInput input(
        R"(toolchain("bad") {
          tool("link") {
            outputs = [ "foo" ]
            runtime_outputs = [ "bar" ]
          }
        })");
    ASSERT_FALSE(input.has_error());

    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    ASSERT_TRUE(err.has_error()) << err.message();
  }
}
