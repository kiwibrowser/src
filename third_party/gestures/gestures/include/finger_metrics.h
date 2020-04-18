// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_FINGER_METRICS_H_
#define GESTURES_FINGER_METRICS_H_

#include <cmath>

#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/vector.h"

namespace gestures {

static const size_t kMaxFingers = 10;
static const size_t kMaxGesturingFingers = 4;
static const size_t kMaxTapFingers = 10;

// A datastructure describing a 2D vector in the mathematical sense.
struct Vector2 {
  Vector2() : x(0), y(0) {}
  Vector2(float x, float y) : x(x), y(y) {}
  Vector2(const Vector2& other) : x(other.x), y(other.y) {}
  explicit Vector2(const FingerState& state) : x(state.position_x),
                                               y(state.position_y) {}

  Vector2 Sub(const Vector2& other) {
    return Vector2(x - other.x, y - other.y);
  }

  Vector2 Add(const Vector2& other) {
    return Vector2(x + other.x, y + other.y);
  }

  float MagSq() const {
    return x * x + y * y;
  }
  float Mag() const {
    return sqrtf(MagSq());
  }
  bool operator==(const Vector2& that) const {
    return x == that.x && y == that.y;
  }
  bool operator!=(const Vector2& that) const {
    return !(*this == that);
  }

  float x;
  float y;
};

extern Vector2 Add(const Vector2& left, const Vector2& right);
extern Vector2 Sub(const Vector2& left, const Vector2& right);
extern float Dot(const Vector2& left, const Vector2& right);

class MetricsProperties {
 public:
  explicit MetricsProperties(PropRegistry* prop_reg);

  // Maximum distance [mm] two fingers may be separated and still be eligible
  // for a two-finger gesture (e.g., scroll / tap / click). These define an
  // ellipse with horizontal and vertical axes lengths (think: radii).
  DoubleProperty two_finger_close_horizontal_distance_thresh;
  DoubleProperty two_finger_close_vertical_distance_thresh;
};

// This class describes a finger and derives additional metrics that
// are useful for gesture recognition.
// In contrast to a FingerState an instance of this class has the
// lifetime of the duration the finger touches the touchpad. This allows
// metrics to be derived from the history of a finger.
class FingerMetrics {
 public:
  FingerMetrics();
  explicit FingerMetrics(short tracking_id);
  FingerMetrics(const FingerState& state, MetricsProperties* properties,
                stime_t timestamp);

  // Update the finger metrics from a FingerState.
  // gesture_start: true if fingers have been added or removed during this
  //                sync.
  void Update(const FingerState& state, stime_t timestamp,
              bool gesture_start);

  short tracking_id() const { return tracking_id_; }

  // current position
  Vector2 position() const { return position_; }

  // position delta between current and last frame
  Vector2 delta() const { return delta_; }

  // origin is the time and position where the finger first touched
  Vector2 origin_position() const { return origin_position_; }
  stime_t origin_time() const { return origin_time_; }
  Vector2 origin_delta() const { return Sub(position_, origin_position_); }

  // start is the time and postion where the fingers were located
  // when the last of all current fingers touched (i.e. the gesture started)
  Vector2 start_position() const { return start_position_; }
  stime_t start_time() const { return start_time_; }
  Vector2 start_delta() const { return Sub(position_, start_position_); }

  // instances with the same tracking id are considered equal.
  bool operator==(const FingerMetrics& other) const {
    return other.tracking_id() == tracking_id_;
  }

 private:
  short tracking_id_;
  Vector2 position_;
  Vector2 delta_;
  Vector2 origin_position_;
  Vector2 start_position_;
  stime_t origin_time_;
  stime_t start_time_;
  MetricsProperties* properties_;
};

// The Metrics class is a container for FingerMetrics and additional
// metrics that are based on the interaction of multiple fingers.
// It is responsible for keeping the FingerMetrics instances updated
// with the latest FingerStates.
class Metrics {
 public:
  Metrics(MetricsProperties* properties);

  bool CloseEnoughToGesture(const Vector2& pos_a,
                            const Vector2& pos_b) const;

  // A collection of FingerMetrics describing the current hardware state.
  // The collection is sorted to yield the oldest finger first.
  vector<FingerMetrics, kMaxFingers>& fingers() { return fingers_; }

  // Find a FingerMetrics instance by it's tracking id.
  // Returns NULL if not found.
  FingerMetrics* GetFinger(short tracking_id) {
      return const_cast<FingerMetrics*>(
          const_cast<const Metrics*>(this)->GetFinger(tracking_id));
  }
  const FingerMetrics* GetFinger(short tracking_id) const;

  FingerMetrics* GetFinger(const FingerState& state);
  const FingerMetrics* GetFinger(const FingerState& state) const;

  // Update the collection of FingerMetrics using information from 'hwstate'.
  void Update(const HardwareState& hwstate);

  // Clear all finger information
  void Clear();

 private:
  vector<FingerMetrics, kMaxFingers> fingers_;

  MetricsProperties* properties_;
  std::unique_ptr<MetricsProperties> own_properties_;
};

}  // namespace gestures

#endif  // GESTURES_FINGER_METRICS_H_
