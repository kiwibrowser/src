// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/multitouch_mouse_interpreter.h"

#include <algorithm>

#include "gestures/include/tracer.h"

namespace gestures {

void Origin::PushGesture(const Gesture& result) {
  if (result.type == kGestureTypeButtonsChange) {
    if (result.details.buttons.up & GESTURES_BUTTON_LEFT)
      button_going_up_left_ = result.end_time;
    if (result.details.buttons.up & GESTURES_BUTTON_MIDDLE)
      button_going_up_middle_ = result.end_time;
    if (result.details.buttons.up & GESTURES_BUTTON_RIGHT)
      button_going_up_right_ = result.end_time;
  }
}

stime_t Origin::ButtonGoingUp(int button) const {
  if (button == GESTURES_BUTTON_LEFT)
    return button_going_up_left_;
  if (button == GESTURES_BUTTON_MIDDLE)
    return button_going_up_middle_;
  if (button == GESTURES_BUTTON_RIGHT)
    return button_going_up_right_;
  return 0;
}

MultitouchMouseInterpreter::MultitouchMouseInterpreter(
    PropRegistry* prop_reg,
    Tracer* tracer)
    : MouseInterpreter(prop_reg, tracer),
      state_buffer_(2),
      scroll_buffer_(15),
      prev_gesture_type_(kGestureTypeNull),
      current_gesture_type_(kGestureTypeNull),
      should_fling_(false),
      scroll_manager_(prop_reg),
      click_buffer_depth_(prop_reg, "Click Buffer Depth", 10),
      click_max_distance_(prop_reg, "Click Max Distance", 1.0),
      click_left_button_going_up_lead_time_(prop_reg,
          "Click Left Button Going Up Lead Time", 0.01),
      click_right_button_going_up_lead_time_(prop_reg,
          "Click Right Button Going Up Lead Time", 0.1),
      min_finger_move_distance_(prop_reg, "Minimum Mouse Finger Move Distance",
                                1.75),
      moving_min_rel_amount_(prop_reg, "Moving Min Rel Magnitude", 0.1) {
  InitName();
  memset(&prev_state_, 0, sizeof(prev_state_));
}

void MultitouchMouseInterpreter::ProduceGesture(const Gesture& gesture) {
  origin_.PushGesture(gesture);
  MouseInterpreter::ProduceGesture(gesture);
}

void MultitouchMouseInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                                       stime_t* timeout) {
  if (!state_buffer_.Get(0)->fingers) {
    Err("Must call SetHardwareProperties() before interpreting anything.");
    return;
  }

  // Should we remove all fingers from our structures, or just removed ones?
  if ((hwstate->rel_x * hwstate->rel_x + hwstate->rel_y * hwstate->rel_y) >
      moving_min_rel_amount_.val_ * moving_min_rel_amount_.val_) {
    start_position_.clear();
    moving_.clear();
    should_fling_ = false;
  } else {
    RemoveMissingIdsFromMap(&start_position_, *hwstate);
    RemoveMissingIdsFromSet(&moving_, *hwstate);
  }

  // Set start positions/moving
  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    const FingerState& fs = hwstate->fingers[i];
    if (MapContainsKey(start_position_, fs.tracking_id)) {
      // Is moving?
      if (!SetContainsValue(moving_, fs.tracking_id) &&  // not already moving &
          start_position_[fs.tracking_id].Sub(Vector2(fs)).MagSq() >=  // moving
          min_finger_move_distance_.val_ * min_finger_move_distance_.val_) {
        moving_.insert(fs.tracking_id);
      }
      continue;
    }
    start_position_[fs.tracking_id] = Vector2(fs);
  }

  // Mark all non-moving fingers as unable to cause scroll
  for (size_t i = 0; i < hwstate->finger_cnt; i++) {
    FingerState* fs = &hwstate->fingers[i];
    if (!SetContainsValue(moving_, fs->tracking_id))
      fs->flags |=
          GESTURES_FINGER_WARP_X_NON_MOVE | GESTURES_FINGER_WARP_Y_NON_MOVE;
  }

  // Record current HardwareState now.
  state_buffer_.PushState(*hwstate);

  // TODO(clchiou): Remove palm and thumb.
  gs_fingers_.clear();
  size_t num_fingers = std::min(kMaxGesturingFingers,
                                (size_t)state_buffer_.Get(0)->finger_cnt);
  const FingerState* fs = state_buffer_.Get(0)->fingers;
  for (size_t i = 0; i < num_fingers; i++)
    gs_fingers_.insert(fs[i].tracking_id);

  InterpretScrollWheelEvent(*hwstate, true);
  InterpretScrollWheelEvent(*hwstate, false);
  InterpretMouseButtonEvent(prev_state_, *state_buffer_.Get(0));
  InterpretMouseMotionEvent(prev_state_, *state_buffer_.Get(0));

  bool should_interpret_multitouch = true;

  // Some mice (Logitech) will interleave finger data and rel data, which can
  // make finger tracking tricky. To avoid problems, if this current frame
  // was rel data, and the previous finger data exactly matches this finger
  // data, we remove the last hardware state from our buffer. This is okay
  // because we already processed the rel data.
  if (state_buffer_.Get(0) && state_buffer_.Get(1)) {
    HardwareState* prev_hs = state_buffer_.Get(1);
    HardwareState* cur_hs = state_buffer_.Get(0);
    bool cur_has_rel = cur_hs->rel_x || cur_hs->rel_y ||
        cur_hs->rel_wheel || cur_hs->rel_hwheel;
    bool different_fingers = prev_hs->touch_cnt != cur_hs->touch_cnt ||
        prev_hs->finger_cnt != cur_hs->finger_cnt;
    if (!different_fingers && cur_has_rel) {
      // Compare actual fingers themselves
      for (size_t i = 0; i < cur_hs->finger_cnt; i++) {
        if (!cur_hs->fingers[i].NonFlagsEquals(prev_hs->fingers[i])) {
          different_fingers = true;
          break;
        }
      }
      if (!different_fingers) {
        state_buffer_.PopState();
        should_interpret_multitouch = false;
      }
    }
  }

  if (should_interpret_multitouch)
    InterpretMultitouchEvent();

  // We don't keep finger data here, this is just for standard mouse:
  prev_state_ = *hwstate;
  prev_state_.fingers = NULL;
  prev_state_.finger_cnt = 0;

  prev_gs_fingers_ = gs_fingers_;
  prev_gesture_type_ = current_gesture_type_;
}

void MultitouchMouseInterpreter::Initialize(
    const HardwareProperties* hw_props,
    Metrics* metrics,
    MetricsProperties* mprops,
    GestureConsumer* consumer) {
  Interpreter::Initialize(hw_props, metrics, mprops, consumer);
  state_buffer_.Reset(hw_props->max_finger_cnt);
}

void MultitouchMouseInterpreter::InterpretMultitouchEvent() {
  Gesture result;

  // If a gesturing finger just left, do fling/lift
  if (should_fling_ && AnyGesturingFingerLeft(*state_buffer_.Get(0),
                                              prev_gs_fingers_)) {
    current_gesture_type_ = kGestureTypeFling;
    scroll_manager_.ComputeFling(state_buffer_, scroll_buffer_, &result);
    if (result.type == kGestureTypeFling)
      result.details.fling.vx = 0.0;
    if (result.details.fling.vy == 0.0)
      result.type = kGestureTypeNull;
    should_fling_ = false;
  } else if (gs_fingers_.size() > 0) {
    // In general, finger movements are interpreted as scroll, but as
    // clicks and scrolls on multi-touch mice are both single-finger
    // gesture, we have to recognize and separate clicks from scrolls,
    // when a user is actually clicking.
    //
    // This is how we do for now: We look for characteristic patterns of
    // clicks, and if we find one, we hold off emitting scroll gesture for
    // a few time frames to prevent premature scrolls.
    //
    // The patterns we look for:
    // * Small finger movements when button is down
    // * Finger movements after button goes up

    bool update_scroll_buffer =
        scroll_manager_.ComputeScroll(state_buffer_,
                                      prev_gs_fingers_,
                                      gs_fingers_,
                                      prev_gesture_type_,
                                      prev_result_,
                                      &result,
                                      &scroll_buffer_);
    current_gesture_type_ = result.type;
    if (current_gesture_type_ == kGestureTypeScroll)
      should_fling_ = true;

    bool hold_off_scroll = false;
    const HardwareState& state = *state_buffer_.Get(0);
    // Check small finger movements when button is down
    if (state.buttons_down) {
      float dist_sq, dt;
      scroll_buffer_.GetSpeedSq(click_buffer_depth_.val_, &dist_sq, &dt);
      if (dist_sq < click_max_distance_.val_ * click_max_distance_.val_)
        hold_off_scroll = true;
    }
    // Check button going up lead time
    stime_t now = state.timestamp;
    stime_t button_left_age =
        now - origin_.ButtonGoingUp(GESTURES_BUTTON_LEFT);
    stime_t button_right_age =
        now - origin_.ButtonGoingUp(GESTURES_BUTTON_RIGHT);
    hold_off_scroll = hold_off_scroll ||
        (button_left_age < click_left_button_going_up_lead_time_.val_) ||
        (button_right_age < click_right_button_going_up_lead_time_.val_);

    if (hold_off_scroll && result.type == kGestureTypeScroll) {
      current_gesture_type_ = kGestureTypeNull;
      result.type = kGestureTypeNull;
    }
    if (current_gesture_type_ == kGestureTypeScroll &&
        !update_scroll_buffer) {
      return;
    }
  }
  scroll_manager_.UpdateScrollEventBuffer(current_gesture_type_,
                                          &scroll_buffer_);
  if (result.type != kGestureTypeNull)
    ProduceGesture(result);
  prev_result_ = result;
}

}  // namespace gestures
