// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_FLING_BOOSTER_H_
#define UI_EVENTS_BLINK_FLING_BOOSTER_H_

#include "third_party/blink/public/platform/web_gesture_event.h"

namespace ui {

// TODO(dcheng): This class should probably be using base::TimeTicks internally.
class FlingBooster {
 public:
  FlingBooster(const gfx::Vector2dF& fling_velocity,
               blink::WebGestureDevice source_device,
               int modifiers);

  // Returns true if the event should be suppressed due to to an active,
  // boost-enabled fling, in which case further processing should cease.
  bool FilterGestureEventForFlingBoosting(
      const blink::WebGestureEvent& gesture_event,
      bool* out_cancel_current_fling);

  bool MustCancelDeferredFling() const;

  void set_last_fling_animation_time(double last_fling_animate_time_seconds) {
    last_fling_animate_time_seconds_ = last_fling_animate_time_seconds;
  }

  gfx::Vector2dF current_fling_velocity() const {
    return current_fling_velocity_;
  }

  void set_current_fling_velocity(const gfx::Vector2dF& fling_velocity) {
    current_fling_velocity_ = fling_velocity;
  }

  bool fling_boosted() const { return fling_boosted_; }

  bool fling_cancellation_is_deferred() const {
    return !!deferred_fling_cancel_time_seconds_;
  }

  blink::WebGestureEvent last_boost_event() const {
    return last_fling_boost_event_;
  }

 private:
  bool ShouldBoostFling(const blink::WebGestureEvent& fling_start_event);

  bool ShouldSuppressScrollForFlingBoosting(
      const blink::WebGestureEvent& scroll_update_event);

  // Set a time in the future after which a boost-enabled fling will terminate
  // without further momentum from the user.
  void ExtendBoostedFlingTimeout(const blink::WebGestureEvent& event);

  gfx::Vector2dF current_fling_velocity_;

  // These store the current active fling source device and modifiers since a
  // new fling start event must have the same source device and modifiers to be
  // able to boost the active fling.
  blink::WebGestureDevice source_device_;
  int modifiers_;

  // Time at which an active fling should expire due to a deferred cancellation
  // event.
  double deferred_fling_cancel_time_seconds_;

  // Time at which the last fling animation has happened.
  double last_fling_animate_time_seconds_;

  // Whether the current active fling is boosted or replaced by a new fling
  // start event.
  bool fling_boosted_;

  // The last event that extended the lifetime of the boosted fling. If the
  // event was a scroll gesture, a GestureScrollBegin needs to be inserted if
  // the fling terminates.
  blink::WebGestureEvent last_fling_boost_event_;

  DISALLOW_COPY_AND_ASSIGN(FlingBooster);
};

}  // namespace ui

#endif  // UI_EVENTS_BLINK_FLING_BOOSTER_H_
