// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "chrome/installer/util/callback_work_item.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A callback that always fails (returns false).
bool TestFailureCallback(const CallbackWorkItem& work_item) {
  return false;
}

}  // namespace

// Test that the work item returns false when a callback returns failure.
TEST(CallbackWorkItemTest, TestFailure) {
  CallbackWorkItem work_item(base::Bind(&TestFailureCallback));

  EXPECT_FALSE(work_item.Do());
}

namespace {

enum TestCallbackState {
  TCS_UNDEFINED,
  TCS_CALLED_FORWARD,
  TCS_CALLED_ROLLBACK,
};

// A callback that sets |state| according to whether it is rolling forward or
// backward.
bool TestForwardBackwardCallback(TestCallbackState* state,
                                 const CallbackWorkItem& work_item) {
  *state = work_item.IsRollback() ? TCS_CALLED_ROLLBACK : TCS_CALLED_FORWARD;
  return true;
}

}  // namespace

// Test that the callback is invoked correclty during Do() and Rollback().
TEST(CallbackWorkItemTest, TestForwardBackward) {
  TestCallbackState state = TCS_UNDEFINED;

  CallbackWorkItem work_item(base::Bind(&TestForwardBackwardCallback, &state));

  EXPECT_TRUE(work_item.Do());
  EXPECT_EQ(TCS_CALLED_FORWARD, state);

  work_item.Rollback();
  EXPECT_EQ(TCS_CALLED_ROLLBACK, state);
}
