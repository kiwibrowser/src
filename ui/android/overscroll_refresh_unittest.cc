// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/scoped_java_ref.h"
#include "cc/layers/layer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/android/overscroll_refresh.h"
#include "ui/android/overscroll_refresh_handler.h"

namespace ui {

class OverscrollRefreshTest : public OverscrollRefreshHandler,
                              public testing::Test {
 public:
  OverscrollRefreshTest() : OverscrollRefreshHandler(nullptr) {}

  // OverscrollRefreshHandler implementation.
  bool PullStart() override {
    started_ = true;
    return true;
  }

  void PullUpdate(float delta) override { delta_ += delta; }

  void PullRelease(bool allow_refresh) override {
    released_ = true;
    refresh_allowed_ = allow_refresh;
  }

  void PullReset() override { reset_ = true; }

  bool GetAndResetPullStarted() {
    bool result = started_;
    started_ = false;
    return result;
  }

  float GetAndResetPullDelta() {
    float result = delta_;
    delta_ = 0;
    return result;
  }

  bool GetAndResetPullReleased() {
    bool result = released_;
    released_ = false;
    return result;
  }

  bool GetAndResetRefreshAllowed() {
    bool result = refresh_allowed_;
    refresh_allowed_ = false;
    return result;
  }

  bool GetAndResetPullReset() {
    bool result = reset_;
    reset_ = false;
    return result;
  }

 private:
  float delta_ = 0;
  bool started_ = false;
  bool released_ = false;
  bool reset_ = false;
  bool refresh_allowed_ = false;
};

TEST_F(OverscrollRefreshTest, Basic) {
  OverscrollRefresh effect(this);

  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());

  effect.OnScrollBegin();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_TRUE(effect.IsAwaitingScrollUpdateAck());

  // The initial scroll should not be consumed, as it should first be offered
  // to content.
  gfx::Vector2dF scroll_up(0, 10);
  EXPECT_FALSE(effect.WillHandleScrollUpdate(scroll_up));
  EXPECT_FALSE(effect.IsActive());
  EXPECT_TRUE(effect.IsAwaitingScrollUpdateAck());

  // The unconsumed, overscrolling scroll will trigger the effect.
  effect.OnOverscrolled();
  EXPECT_TRUE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_TRUE(GetAndResetPullStarted());

  // Further scrolls will be consumed.
  EXPECT_TRUE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 50)));
  EXPECT_EQ(50.f, GetAndResetPullDelta());
  EXPECT_TRUE(effect.IsActive());

  // Even scrolls in the down direction should be consumed.
  EXPECT_TRUE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, -50)));
  EXPECT_EQ(-50.f, GetAndResetPullDelta());
  EXPECT_TRUE(effect.IsActive());

  // Ending the scroll while beyond the threshold should trigger a refresh.
  gfx::Vector2dF zero_velocity;
  EXPECT_FALSE(GetAndResetPullReleased());
  effect.OnScrollEnd(zero_velocity);
  EXPECT_FALSE(effect.IsActive());
  EXPECT_TRUE(GetAndResetPullReleased());
  EXPECT_TRUE(GetAndResetRefreshAllowed());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfInitialYOffsetIsNotZero) {
  OverscrollRefresh effect(this);

  // A positive y scroll offset at the start of scroll will prevent activation,
  // even if the subsequent scroll overscrolls upward.
  gfx::Vector2dF nonzero_offset(0, 10);
  bool overflow_y_hidden = false;
  effect.OnFrameUpdated(nonzero_offset, overflow_y_hidden);
  effect.OnScrollBegin();

  effect.OnFrameUpdated(gfx::Vector2dF(), overflow_y_hidden);
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  effect.OnOverscrolled();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 500)));
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_FALSE(GetAndResetPullStarted());
  EXPECT_FALSE(GetAndResetPullReleased());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfOverflowYHidden) {
  OverscrollRefresh effect(this);

  // overflow-y:hidden at the start of scroll will prevent activation.
  gfx::Vector2dF zero_offset;
  bool overflow_y_hidden = true;
  effect.OnFrameUpdated(zero_offset, overflow_y_hidden);
  effect.OnScrollBegin();

  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  effect.OnOverscrolled();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 500)));
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_FALSE(GetAndResetPullStarted());
  EXPECT_FALSE(GetAndResetPullReleased());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfInitialScrollDownward) {
  OverscrollRefresh effect(this);
  effect.OnScrollBegin();

  // A downward initial scroll will prevent activation, even if the subsequent
  // scroll overscrolls upward.
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, -10)));
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());

  effect.OnOverscrolled();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 500)));
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_FALSE(GetAndResetPullReleased());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfInitialScrollOrTouchConsumed) {
  OverscrollRefresh effect(this);
  effect.OnScrollBegin();
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  ASSERT_TRUE(effect.IsAwaitingScrollUpdateAck());

  // Consumption of the initial touchmove or scroll should prevent future
  // activation.
  effect.Reset();
  effect.OnOverscrolled();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 500)));
  effect.OnOverscrolled();
  EXPECT_FALSE(effect.IsActive());
  EXPECT_FALSE(effect.IsAwaitingScrollUpdateAck());
  EXPECT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 500)));
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_FALSE(GetAndResetPullStarted());
  EXPECT_FALSE(GetAndResetPullReleased());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfFlungDownward) {
  OverscrollRefresh effect(this);
  effect.OnScrollBegin();
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  ASSERT_TRUE(effect.IsAwaitingScrollUpdateAck());
  effect.OnOverscrolled();
  ASSERT_TRUE(effect.IsActive());
  EXPECT_TRUE(GetAndResetPullStarted());

  // Terminating the pull with a down-directed fling should prevent triggering.
  effect.OnScrollEnd(gfx::Vector2dF(0, -1000));
  EXPECT_TRUE(GetAndResetPullReleased());
  EXPECT_FALSE(GetAndResetRefreshAllowed());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfReleasedWithoutActivation) {
  OverscrollRefresh effect(this);
  effect.OnScrollBegin();
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  ASSERT_TRUE(effect.IsAwaitingScrollUpdateAck());
  effect.OnOverscrolled();
  ASSERT_TRUE(effect.IsActive());
  EXPECT_TRUE(GetAndResetPullStarted());

  // An early release should prevent the refresh action from firing.
  effect.ReleaseWithoutActivation();
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_TRUE(GetAndResetPullReleased());
  EXPECT_FALSE(GetAndResetRefreshAllowed());
}

TEST_F(OverscrollRefreshTest, NotTriggeredIfReset) {
  OverscrollRefresh effect(this);
  effect.OnScrollBegin();
  ASSERT_FALSE(effect.WillHandleScrollUpdate(gfx::Vector2dF(0, 10)));
  ASSERT_TRUE(effect.IsAwaitingScrollUpdateAck());
  effect.OnOverscrolled();
  ASSERT_TRUE(effect.IsActive());
  EXPECT_TRUE(GetAndResetPullStarted());

  // An early reset should prevent the refresh action from firing.
  effect.Reset();
  EXPECT_TRUE(GetAndResetPullReset());
  effect.OnScrollEnd(gfx::Vector2dF());
  EXPECT_FALSE(GetAndResetPullReleased());
}

}  // namespace ui
