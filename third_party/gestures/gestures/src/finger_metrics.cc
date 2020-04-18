// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/finger_metrics.h"

namespace gestures {

Vector2 Add(const Vector2& left, const Vector2& right) {
  return Vector2(left).Add(right);
}

Vector2 Sub(const Vector2& left, const Vector2& right) {
  return Vector2(left).Sub(right);
}

float Dot(const Vector2& left, const Vector2& right) {
  return left.x * right.x +  left.y * right.y;
}

MetricsProperties::MetricsProperties(PropRegistry* prop_reg)
    : two_finger_close_horizontal_distance_thresh(
          prop_reg,
          "Two Finger Horizontal Close Distance Thresh",
          50.0),
      two_finger_close_vertical_distance_thresh(
          prop_reg,
          "Two Finger Vertical Close Distance Thresh",
          45.0) {}

FingerMetrics::FingerMetrics()
    : tracking_id_(-1) {}

FingerMetrics::FingerMetrics(short tracking_id)
    : tracking_id_(tracking_id) {}

FingerMetrics::FingerMetrics(const FingerState& state,
                             MetricsProperties* properties,
                             stime_t timestamp)
    : tracking_id_(state.tracking_id),
      position_(state.position_x, state.position_y),
      origin_position_(state.position_x, state.position_y),
      origin_time_(timestamp),
      properties_(properties) {}

void FingerMetrics::Update(const FingerState& state, stime_t timestamp,
                           bool gesture_start) {
  Vector2 new_position = Vector2(state.position_x, state.position_y);
  delta_ = Sub(new_position, position_);
  position_ = new_position;

  if (gesture_start) {
    start_position_ = position_;
    start_time_ = timestamp;
  }
}

bool Metrics::CloseEnoughToGesture(const Vector2& pos_a,
                                   const Vector2& pos_b) const {
  float horiz_axis_sq =
      properties_->two_finger_close_horizontal_distance_thresh.val_ *
      properties_->two_finger_close_horizontal_distance_thresh.val_;
  float vert_axis_sq =
      properties_->two_finger_close_vertical_distance_thresh.val_ *
      properties_->two_finger_close_vertical_distance_thresh.val_;
  Vector2 delta = Sub(pos_a, pos_b);
  // Equation of ellipse:
  //    ,.--+--..
  //  ,'   V|    `.   x^2   y^2
  // |      +------|  --- + --- < 1
  //  \        H  /   H^2   V^2
  //   `-..__,,.-'
  return vert_axis_sq * delta.x * delta.x + horiz_axis_sq * delta.y * delta.y
         < vert_axis_sq * horiz_axis_sq;
}

Metrics::Metrics(MetricsProperties* properties) : properties_(properties) {}

const FingerMetrics* Metrics::GetFinger(short tracking_id) const {
  auto iter = fingers_.find(FingerMetrics(tracking_id));
  if (iter != fingers_.end())
    return iter;
  else
    return NULL;
}

const FingerMetrics* Metrics::GetFinger(const FingerState& state) const {
  return GetFinger(state.tracking_id);
}

void Metrics::Update(const HardwareState& hwstate) {
  int previous_count = fingers_.size();
  int existing_count = 0;
  int new_count = 0;

  // create metrics for new fingers
  for (int i=0; i<hwstate.finger_cnt; ++i) {
    const FingerState& state = hwstate.fingers[i];
    auto iter = fingers_.find(FingerMetrics(state.tracking_id));
    if (iter == fingers_.end()) {
      fingers_.push_back(FingerMetrics(state, properties_,
                                       hwstate.timestamp));
      ++new_count;
    } else {
      ++existing_count;
    }
  }

  // remove metrics for lifted fingers
  if (existing_count != previous_count) {
    auto iter = fingers_.begin();
    while (iter != fingers_.end()) {
      if (!hwstate.GetFingerState(iter->tracking_id()))
        iter = fingers_.erase(iter);
      else
        ++iter;
    }
  }

  // when a new finger has been added or a finger has been removed
  // we consider this to be a new gesture starting
  bool gesture_start = (existing_count != previous_count) || new_count > 0;
  for (FingerMetrics& finger: fingers_) {
    const FingerState* fs = hwstate.GetFingerState(finger.tracking_id());
    if (!fs) {
      Err("Unexpected missing finger state!");
      continue;
    }
    finger.Update(*fs, hwstate.timestamp, gesture_start);
  }
}

void Metrics::Clear() {
  fingers_.clear();
}

}  // namespace gestures
