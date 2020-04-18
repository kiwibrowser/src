// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/bubble/bubble_manager_mocks.h"

#include "base/memory/ptr_util.h"

MockBubbleUi::MockBubbleUi() {}

MockBubbleUi::~MockBubbleUi() { Destroyed(); }

MockBubbleDelegate::MockBubbleDelegate() : bubble_ui_(new MockBubbleUi) {}

MockBubbleDelegate::~MockBubbleDelegate() { Destroyed(); }

// static
std::unique_ptr<MockBubbleDelegate> MockBubbleDelegate::Default() {
  MockBubbleDelegate* delegate = new MockBubbleDelegate;
  EXPECT_CALL(*delegate, ShouldClose(testing::_))
      .Times(testing::AtMost(1))
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*delegate, DidClose(testing::_));
  EXPECT_CALL(*delegate, Destroyed());
  return base::WrapUnique(delegate);
}

// static
std::unique_ptr<MockBubbleDelegate> MockBubbleDelegate::Stubborn() {
  MockBubbleDelegate* delegate = new MockBubbleDelegate;
  EXPECT_CALL(*delegate, ShouldClose(testing::_))
      .WillRepeatedly(testing::Return(false));
  EXPECT_CALL(*delegate, DidClose(testing::_));
  EXPECT_CALL(*delegate, Destroyed());
  return base::WrapUnique(delegate);
}
