// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/fling_booster.h"

#include <memory>

#include "base/test/simple_test_tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event_modifiers.h"

using blink::WebGestureDevice;
using blink::WebGestureEvent;
using blink::WebInputEvent;

namespace ui {
namespace test {

class FlingBoosterTest : public testing::Test {
 public:
  FlingBoosterTest() : delta_time_(base::TimeDelta::FromMilliseconds(10)) {
    gesture_scroll_event_.SetSourceDevice(blink::kWebGestureDeviceTouchscreen);
  }

  WebGestureEvent CreateFlingStart(base::TimeTicks timestamp,
                                   WebGestureDevice source_device,
                                   const gfx::Vector2dF& velocity,
                                   int modifiers) {
    WebGestureEvent fling_start(WebInputEvent::kGestureFlingStart, modifiers,
                                timestamp, source_device);
    fling_start.data.fling_start.velocity_x = velocity.x();
    fling_start.data.fling_start.velocity_y = velocity.y();
    return fling_start;
  }

  WebGestureEvent CreateFlingCancel(base::TimeTicks timestamp,
                                    WebGestureDevice source_device) {
    WebGestureEvent fling_cancel(WebInputEvent::kGestureFlingCancel, 0,
                                 timestamp, source_device);
    return fling_cancel;
  }

  void StartFirstFling() {
    event_time_ = base::TimeTicks() + delta_time_;
    fling_booster_.reset(new FlingBooster(
        gfx::Vector2dF(1000, 1000), blink::kWebGestureDeviceTouchscreen, 0));
    fling_booster_->set_last_fling_animation_time(
        EventTimeStampToSeconds(event_time_));
  }

  void CancelFling() {
    WebGestureEvent fling_cancel_event =
        CreateFlingCancel(event_time_, blink::kWebGestureDeviceTouchscreen);
    bool cancel_current_fling;
    EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
        fling_cancel_event, &cancel_current_fling));
    EXPECT_FALSE(cancel_current_fling);
    EXPECT_TRUE(fling_booster_->fling_cancellation_is_deferred());
  }

 protected:
  std::unique_ptr<FlingBooster> fling_booster_;
  base::TimeDelta delta_time_;
  base::TimeTicks event_time_;
  WebGestureEvent gesture_scroll_event_;
};

TEST_F(FlingBoosterTest, FlingBoost) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // The GestureScrollBegin should be swallowed by the fling when a fling
  // cancellation is deferred.
  gesture_scroll_event_.SetTimeStamp(event_time_);
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollBegin);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);

  // Animate calls within the deferred cancellation window should continue.
  event_time_ += delta_time_;
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_FALSE(fling_booster_->MustCancelDeferredFling());

  // GestureScrollUpdates in the same direction and at sufficient speed should
  // be swallowed by the fling.
  gesture_scroll_event_.SetTimeStamp(event_time_);
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollUpdate);
  gesture_scroll_event_.data.scroll_update.delta_x = 100;
  gesture_scroll_event_.data.scroll_update.delta_y = 100;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);

  // Animate calls within the deferred cancellation window should continue.
  event_time_ += delta_time_;
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_FALSE(fling_booster_->MustCancelDeferredFling());

  // GestureFlingStart in the same direction and at sufficient speed should
  // boost the active fling.
  WebGestureEvent fling_start_event =
      CreateFlingStart(event_time_, blink::kWebGestureDeviceTouchscreen,
                       gfx::Vector2dF(1000, 1000), 0);
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_start_event, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);
  EXPECT_EQ(gfx::Vector2dF(2000, 2000),
            fling_booster_->current_fling_velocity());
  EXPECT_TRUE(fling_booster_->fling_boosted());

  // Animate calls within the deferred cancellation window should continue.
  event_time_ += delta_time_;
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_FALSE(fling_booster_->MustCancelDeferredFling());

  // GestureFlingCancel should terminate the fling if no boosting gestures are
  // received within the timeout window.
  CancelFling();
  event_time_ += base::TimeDelta::FromMilliseconds(100);
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_TRUE(fling_booster_->MustCancelDeferredFling());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfScrollDelayed) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // The GestureScrollBegin should be swallowed by the fling when a fling
  // cancellation is deferred.
  gesture_scroll_event_.SetTimeStamp(event_time_);
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollBegin);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);

  // If no GestureScrollUpdate or GestureFlingStart is received within the
  // timeout window, the fling should be cancelled and scrolling should resume.
  event_time_ += base::TimeDelta::FromMilliseconds(100);
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_TRUE(fling_booster_->MustCancelDeferredFling());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfNotAnimated) {
  StartFirstFling();

  // Animate fling once.
  event_time_ += delta_time_;
  fling_booster_->set_last_fling_animation_time(
      EventTimeStampToSeconds(event_time_));
  EXPECT_FALSE(fling_booster_->MustCancelDeferredFling());

  // Cancel the fling after long delay of no animate. The fling cancellation
  // should be deferred to allow fling boosting events to arrive.
  event_time_ += base::TimeDelta::FromMilliseconds(100);
  CancelFling();

  // The GestureScrollBegin should be swallowed by the fling when a fling
  // cancellation is deferred.
  gesture_scroll_event_.SetTimeStamp(event_time_);
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollBegin);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);

  // Should exit scroll boosting on GestureScrollUpdate due to long delay since
  // last animate and cancel old fling. The scroll update event shouldn't get
  // filtered.
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollUpdate);
  gesture_scroll_event_.data.scroll_update.delta_y = 100;
  EXPECT_FALSE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_TRUE(cancel_current_fling);
}

TEST_F(FlingBoosterTest, NoFlingBoostIfFlingInDifferentDirection) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // If the new fling is orthogonal to the existing fling, no boosting should
  // take place, with the new fling replacing the old.
  WebGestureEvent fling_start_event =
      CreateFlingStart(event_time_, blink::kWebGestureDeviceTouchscreen,
                       gfx::Vector2dF(-1000, -1000), 0);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_start_event, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);
  EXPECT_EQ(gfx::Vector2dF(-1000, -1000),
            fling_booster_->current_fling_velocity());
  EXPECT_FALSE(fling_booster_->fling_boosted());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfScrollInDifferentDirection) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // If the GestureScrollUpdate is in a different direction than the fling,
  // the fling should be cancelled and the update event shouldn't get filtered.
  gesture_scroll_event_.SetTimeStamp(event_time_);
  gesture_scroll_event_.SetType(WebInputEvent::kGestureScrollUpdate);
  gesture_scroll_event_.data.scroll_update.delta_x = -100;
  bool cancel_current_fling;
  EXPECT_FALSE(fling_booster_->FilterGestureEventForFlingBoosting(
      gesture_scroll_event_, &cancel_current_fling));
  EXPECT_TRUE(cancel_current_fling);
}

TEST_F(FlingBoosterTest, NoFlingBoostIfFlingTooSlow) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // If the new fling velocity is too small, no boosting should take place, with
  // the new fling replacing the old.
  WebGestureEvent fling_start_event =
      CreateFlingStart(event_time_, blink::kWebGestureDeviceTouchscreen,
                       gfx::Vector2dF(100, 100), 0);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_start_event, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);
  EXPECT_EQ(gfx::Vector2dF(100, 100), fling_booster_->current_fling_velocity());
  EXPECT_FALSE(fling_booster_->fling_boosted());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfPreventBoostingFlagIsSet) {
  StartFirstFling();

  // The fling cancellation should not be deferred because of prevent boosting
  // flag set.
  WebGestureEvent fling_cancel_event =
      CreateFlingCancel(event_time_, blink::kWebGestureDeviceTouchscreen);
  fling_cancel_event.data.fling_cancel.prevent_boosting = true;
  bool cancel_current_fling;
  EXPECT_FALSE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_cancel_event, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);
  EXPECT_FALSE(fling_booster_->fling_cancellation_is_deferred());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfDifferentFlingModifiers) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // GestureFlingStart with different modifiers should replace the old fling.
  WebGestureEvent fling_start_event =
      CreateFlingStart(event_time_, blink::kWebGestureDeviceTouchscreen,
                       gfx::Vector2dF(500, 500), MODIFIER_SHIFT);
  bool cancel_current_fling;
  EXPECT_TRUE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_start_event, &cancel_current_fling));
  EXPECT_FALSE(cancel_current_fling);
  EXPECT_EQ(gfx::Vector2dF(500, 500), fling_booster_->current_fling_velocity());
  EXPECT_FALSE(fling_booster_->fling_boosted());
}

TEST_F(FlingBoosterTest, NoFlingBoostIfDifferentFlingSourceDevices) {
  StartFirstFling();

  // The fling cancellation should be deferred to allow fling boosting events to
  // arrive.
  CancelFling();

  // GestureFlingStart with different source device should not get filtered by
  // fling_booster.
  WebGestureEvent fling_start_event =
      CreateFlingStart(event_time_, blink::kWebGestureDeviceTouchpad,
                       gfx::Vector2dF(500, 500), 0);
  bool cancel_current_fling;
  EXPECT_FALSE(fling_booster_->FilterGestureEventForFlingBoosting(
      fling_start_event, &cancel_current_fling));
  EXPECT_TRUE(cancel_current_fling);
}

}  // namespace test
}  // namespace ui
