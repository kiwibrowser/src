// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/async_document_subresource_filter_test_utils.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace subresource_filter {
namespace testing {

TestActivationStateCallbackReceiver::TestActivationStateCallbackReceiver() =
    default;
TestActivationStateCallbackReceiver::~TestActivationStateCallbackReceiver() =
    default;

base::Callback<void(ActivationState)>
TestActivationStateCallbackReceiver::GetCallback() {
  return base::Bind(&TestActivationStateCallbackReceiver::Callback,
                    base::Unretained(this));
}

void TestActivationStateCallbackReceiver::WaitForActivationDecision() {
  ASSERT_EQ(0, callback_count_);
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();
  run_loop.Run();
}

void TestActivationStateCallbackReceiver::ExpectReceivedOnce(
    const ActivationState& expected_state) const {
  ASSERT_EQ(1, callback_count_);
  EXPECT_EQ(expected_state, last_activation_state_);
}

void TestActivationStateCallbackReceiver::Callback(
    ActivationState activation_state) {
  ++callback_count_;
  last_activation_state_ = activation_state;
  if (quit_closure_)
    quit_closure_.Run();
}

}  // namespace testing
}  // namespace subresource_filter
