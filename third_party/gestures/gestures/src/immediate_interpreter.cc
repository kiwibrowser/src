// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/immediate_interpreter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <tuple>

#include "gestures/include/gestures.h"
#include "gestures/include/logging.h"
#include "gestures/include/util.h"

using std::bind1st;
using std::for_each;
using std::make_pair;
using std::make_tuple;
using std::max;
using std::mem_fun;
using std::min;
using std::tuple;

namespace gestures {

namespace {

float MaxMag(float a, float b) {
  if (fabsf(a) > fabsf(b))
    return a;
  return b;
}
float MinMag(float a, float b) {
  if (fabsf(a) < fabsf(b))
    return a;
  return b;
}

// A comparator class for use with STL algorithms that sorts FingerStates
// by their origin timestamp.
class FingerOriginCompare {
 public:
  explicit FingerOriginCompare(const ImmediateInterpreter* interpreter)
      : interpreter_(interpreter) {
  }
  bool operator() (const FingerState* a, const FingerState* b) const {
    return interpreter_->finger_origin_timestamp(a->tracking_id) <
           interpreter_->finger_origin_timestamp(b->tracking_id);
  }

 private:
  const ImmediateInterpreter* interpreter_;
};

}  // namespace {}

void TapRecord::NoteTouch(short the_id, const FingerState& fs) {
  // New finger must be close enough to an existing finger
  if (!touched_.empty()) {
    bool reject_new_finger = true;
    for (map<short, FingerState, kMaxTapFingers>::const_iterator it =
             touched_.begin(), e = touched_.end(); it != e; ++it) {
      const FingerState& existing_fs = (*it).second;
      if (immediate_interpreter_->metrics_->CloseEnoughToGesture(
              Vector2(existing_fs),
              Vector2(fs))) {
        reject_new_finger = false;
        break;
      }
    }
    if (reject_new_finger)
      return;
  }
  touched_[the_id] = fs;
}

void TapRecord::NoteRelease(short the_id) {
  if (touched_.find(the_id) != touched_.end())
    released_.insert(the_id);
}

void TapRecord::Remove(short the_id) {
  min_tap_pressure_met_.erase(the_id);
  min_cotap_pressure_met_.erase(the_id);
  touched_.erase(the_id);
  released_.erase(the_id);
}

float TapRecord::CotapMinPressure() const {
  return immediate_interpreter_->tap_min_pressure() * 0.5;
}

void TapRecord::Update(const HardwareState& hwstate,
                       const HardwareState& prev_hwstate,
                       const set<short, kMaxTapFingers>& added,
                       const set<short, kMaxTapFingers>& removed,
                       const set<short, kMaxFingers>& dead) {
  if (!t5r2_ && (hwstate.finger_cnt != hwstate.touch_cnt ||
                 prev_hwstate.finger_cnt != prev_hwstate.touch_cnt)) {
    // switch to T5R2 mode
    t5r2_ = true;
    t5r2_touched_size_ = touched_.size();
    t5r2_released_size_ = released_.size();
  }
  if (t5r2_) {
    short diff = static_cast<short>(hwstate.touch_cnt) -
        static_cast<short>(prev_hwstate.touch_cnt);
    if (diff > 0)
      t5r2_touched_size_ += diff;
    else if (diff < 0)
      t5r2_released_size_ += -diff;
  }
  for (set<short, kMaxTapFingers>::const_iterator it = added.begin(),
           e = added.end(); it != e; ++it)
    Log("TapRecord::Update: Added: %d", *it);
  for (set<short, kMaxTapFingers>::const_iterator it = removed.begin(),
           e = removed.end(); it != e; ++it)
    Log("TapRecord::Update: Removed: %d", *it);
  for (set<short, kMaxFingers>::const_iterator it = dead.begin(),
           e = dead.end(); it != e; ++it)
    Log("TapRecord::Update: Dead: %d", *it);
  for_each(dead.begin(), dead.end(),
           bind1st(mem_fun(&TapRecord::Remove), this));
  for (set<short, kMaxTapFingers>::const_iterator it = added.begin(),
           e = added.end(); it != e; ++it)
    NoteTouch(*it, *hwstate.GetFingerState(*it));
  for_each(removed.begin(), removed.end(),
           bind1st(mem_fun(&TapRecord::NoteRelease), this));
  // Check if min tap/cotap pressure met yet
  const float cotap_min_pressure = CotapMinPressure();
  for (map<short, FingerState, kMaxTapFingers>::iterator it =
           touched_.begin(), e = touched_.end();
       it != e; ++it) {
    const FingerState* fs = hwstate.GetFingerState((*it).first);
    if (fs) {
      if (fs->pressure >= immediate_interpreter_->tap_min_pressure())
        min_tap_pressure_met_.insert(fs->tracking_id);
      if (fs->pressure >= cotap_min_pressure) {
        min_cotap_pressure_met_.insert(fs->tracking_id);
        if ((*it).second.pressure < cotap_min_pressure) {
          // Update existing record, since the old one hadn't met the cotap
          // pressure
          (*it).second = *fs;
        }
      }
      stime_t finger_age = hwstate.timestamp -
          immediate_interpreter_->finger_origin_timestamp(fs->tracking_id);
      if (finger_age > immediate_interpreter_->tap_max_finger_age())
        fingers_below_max_age_ = false;
    }
  }
}

void TapRecord::Clear() {
  min_tap_pressure_met_.clear();
  min_cotap_pressure_met_.clear();
  t5r2_ = false;
  t5r2_touched_size_ = 0;
  t5r2_released_size_ = 0;
  fingers_below_max_age_ = true;
  touched_.clear();
  released_.clear();
}

bool TapRecord::Moving(const HardwareState& hwstate,
                       const float dist_max) const {
  const float cotap_min_pressure = CotapMinPressure();
  for (map<short, FingerState, kMaxTapFingers>::const_iterator it =
           touched_.begin(), e = touched_.end(); it != e; ++it) {
    const FingerState* fs = hwstate.GetFingerState((*it).first);
    if (!fs)
      continue;
    // Only look for moving when current frame meets cotap pressure and
    // our history contains a contact that's met cotap pressure.
    if (fs->pressure < cotap_min_pressure ||
        (*it).second.pressure < cotap_min_pressure)
      continue;
    // Compute distance moved
    float dist_x = fs->position_x - (*it).second.position_x;
    float dist_y = fs->position_y - (*it).second.position_y;
    // Respect WARP flags
    if (fs->flags & GESTURES_FINGER_WARP_X_TAP_MOVE)
      dist_x = 0.0;
    if (fs->flags & GESTURES_FINGER_WARP_X_TAP_MOVE)
      dist_y = 0.0;

    bool moving =
        dist_x * dist_x + dist_y * dist_y > dist_max * dist_max;
    if (moving)
      return true;
  }
  return false;
}

bool TapRecord::Motionless(const HardwareState& hwstate, const HardwareState&
                           prev_hwstate, const float max_speed) const {
  const float cotap_min_pressure = CotapMinPressure();
  for (map<short, FingerState, kMaxTapFingers>::const_iterator it =
           touched_.begin(), e = touched_.end(); it != e; ++it) {
    const FingerState* fs = hwstate.GetFingerState((*it).first);
    const FingerState* prev_fs = prev_hwstate.GetFingerState((*it).first);
    if (!fs || !prev_fs)
      continue;
    // Only look for moving when current frame meets cotap pressure and
    // our history contains a contact that's met cotap pressure.
    if (fs->pressure < cotap_min_pressure ||
        prev_fs->pressure < cotap_min_pressure)
      continue;
    // Compute distance moved
    if (DistSq(*fs, *prev_fs) > max_speed * max_speed)
      return false;
  }
  return true;
}

bool TapRecord::TapBegan() const {
  if (t5r2_)
    return t5r2_touched_size_ > 0;
  return !touched_.empty();
}

bool TapRecord::TapComplete() const {
  bool ret = false;
  if (t5r2_)
    ret = t5r2_touched_size_ && t5r2_touched_size_ == t5r2_released_size_;
  else
    ret = !touched_.empty() && (touched_.size() == released_.size());
  for (map<short, FingerState, kMaxTapFingers>::const_iterator
           it = touched_.begin(), e = touched_.end(); it != e; ++it)
    Log("TapRecord::TapComplete: touched_: %d", (*it).first);
  for (set<short, kMaxTapFingers>::const_iterator it = released_.begin(),
           e = released_.end(); it != e; ++it)
    Log("TapRecord::TapComplete: released_: %d", *it);
  return ret;
}

bool TapRecord::MinTapPressureMet() const {
  // True if any touching finger met minimum pressure
  return t5r2_ || !min_tap_pressure_met_.empty();
}

bool TapRecord::FingersBelowMaxAge() const {
  return fingers_below_max_age_;
}

int TapRecord::TapType() const {
  size_t touched_size =
      t5r2_ ? t5r2_touched_size_ : min_cotap_pressure_met_.size();
  int ret = GESTURES_BUTTON_LEFT;
  if (touched_size > 1)
    ret = GESTURES_BUTTON_RIGHT;
  if (touched_size == 3 &&
      immediate_interpreter_->three_finger_click_enable_.val_ &&
      (!t5r2_ || immediate_interpreter_->t5r2_three_finger_click_enable_.val_))
    ret = GESTURES_BUTTON_MIDDLE;
  return ret;
}

// static
ScrollEvent ScrollEvent::Add(const ScrollEvent& evt_a,
                             const ScrollEvent& evt_b) {
  ScrollEvent ret = { evt_a.dx + evt_b.dx,
                      evt_a.dy + evt_b.dy,
                      evt_a.dt + evt_b.dt };
  return ret;
}

void ScrollEventBuffer::Insert(float dx, float dy, float dt) {
  head_ = (head_ + max_size_ - 1) % max_size_;
  buf_[head_].dx = dx;
  buf_[head_].dy = dy;
  buf_[head_].dt = dt;
  size_ = std::min(size_ + 1, max_size_);
}

void ScrollEventBuffer::Clear() {
  size_ = 0;
}

const ScrollEvent& ScrollEventBuffer::Get(size_t offset) const {
  if (offset >= size_) {
    Err("Out of bounds access!");
    // avoid returning null pointer
    static ScrollEvent dummy_event = { 0.0, 0.0, 0.0 };
    return dummy_event;
  }
  return buf_[(head_ + offset) % max_size_];
}

void ScrollEventBuffer::GetSpeedSq(size_t num_events, float* dist_sq,
                                   float* dt) const {
  float dx = 0.0;
  float dy = 0.0;
  *dt = 0.0;
  for (size_t i = 0; i < Size() && i < num_events; i++) {
    const ScrollEvent& evt = Get(i);
    dx += evt.dx;
    dy += evt.dy;
    *dt += evt.dt;
  }
  *dist_sq = dx * dx + dy * dy;
}

HardwareStateBuffer::HardwareStateBuffer(size_t size)
    : states_(new HardwareState[size]),
      newest_index_(0), size_(size), max_finger_cnt_(0) {
  for (size_t i = 0; i < size_; i++) {
    memset(&states_[i], 0, sizeof(HardwareState));
  }
}

HardwareStateBuffer::~HardwareStateBuffer() {
  for (size_t i = 0; i < size_; i++) {
    delete[] states_[i].fingers;
  }
}

void HardwareStateBuffer::Reset(size_t max_finger_cnt) {
  max_finger_cnt_ = max_finger_cnt;
  for (size_t i = 0; i < size_; i++) {
    delete[] states_[i].fingers;
  }
  if (max_finger_cnt_) {
    for (size_t i = 0; i < size_; i++) {
      states_[i].fingers = new FingerState[max_finger_cnt_];
      memset(states_[i].fingers, 0, sizeof(FingerState) * max_finger_cnt_);
    }
  } else {
    for (size_t i = 0; i < size_; i++) {
      states_[i].fingers = NULL;
    }
  }
}

void HardwareStateBuffer::PushState(const HardwareState& state) {
  newest_index_ = (newest_index_ + size_ - 1) % size_;
  Get(0)->DeepCopy(state, max_finger_cnt_);
}

void HardwareStateBuffer::PopState() {
  newest_index_ = (newest_index_ + 1) % size_;
}

ScrollManager::ScrollManager(PropRegistry* prop_reg)
    : prev_result_suppress_finger_movement_(false),
      did_generate_scroll_(false),
      max_stationary_move_speed_(prop_reg, "Max Stationary Move Speed", 0.0),
      max_stationary_move_speed_hysteresis_(
          prop_reg, "Max Stationary Move Speed Hysteresis", 0.0),
      max_stationary_move_suppress_distance_(
          prop_reg, "Max Stationary Move Suppress Distance", 1.0),
      max_pressure_change_(prop_reg, "Max Allowed Pressure Change Per Sec",
                           800.0),
      max_pressure_change_hysteresis_(prop_reg,
                                      "Max Hysteresis Pressure Per Sec",
                                      600.0),
      min_scroll_dead_reckoning_(prop_reg,
                                 "Min Scroll Dead Reckoning Distance",
                                 0.0),
      max_pressure_change_duration_(prop_reg,
                                    "Max Pressure Change Duration",
                                    0.016),
      max_stationary_speed_(prop_reg, "Max Finger Stationary Speed", 0.0),
      vertical_scroll_snap_slope_(prop_reg, "Vertical Scroll Snap Slope",
                                  tanf(DegToRad(50.0))),  // 50 deg. from horiz.
      horizontal_scroll_snap_slope_(prop_reg, "Horizontal Scroll Snap Slope",
                                    tanf(DegToRad(30.0))),

      fling_buffer_depth_(prop_reg, "Fling Buffer Depth", 10),
      fling_buffer_suppress_zero_length_scrolls_(
          prop_reg, "Fling Buffer Suppress Zero Length Scrolls", 1),
      fling_buffer_min_avg_speed_(prop_reg,
                                  "Fling Buffer Min Avg Speed",
                                  10.0) {
}

bool ScrollManager::StationaryFingerPressureChangingSignificantly(
    const HardwareStateBuffer& state_buffer,
    const FingerState& current) const {
  bool pressure_is_increasing = false;
  bool pressure_direction_established = false;
  const FingerState* prev = &current;
  stime_t now = state_buffer.Get(0)->timestamp;
  stime_t duration = 0.0;

  if (max_pressure_change_duration_.val_ > 0.0) {
    for (size_t i = 1; i < state_buffer.Size(); i++) {
      const HardwareState& state = *state_buffer.Get(i);
      stime_t local_duration = now - state.timestamp;
      if (local_duration > max_pressure_change_duration_.val_)
        break;

      duration = local_duration;
      const FingerState* fs = state.GetFingerState(current.tracking_id);
      // If the finger just appeared, skip to check pressure change then
      if (!fs)
        break;

      float pressure_difference = prev->pressure - fs->pressure;
      if (pressure_difference) {
        bool is_currently_increasing = pressure_difference > 0.0;
        if (!pressure_direction_established) {
          pressure_is_increasing = is_currently_increasing;
          pressure_direction_established = true;
        }

        // If pressure changes are unstable, it's likely just noise.
        if (is_currently_increasing != pressure_is_increasing)
          return false;
      }
      prev = fs;
    }
  } else {
    // To disable this feature, max_pressure_change_duration_ can be set to a
    // negative number. When this occurs it reverts to just checking the last
    // event, not looking through the backlog as well.
    prev = state_buffer.Get(1)->GetFingerState(current.tracking_id);
    duration = now - state_buffer.Get(1)->timestamp;
  }

  if (max_stationary_speed_.val_ != 0.0) {
    // If finger moves too fast, we don't consider it stationary.
    float dist_sq = (current.position_x - prev->position_x) *
                    (current.position_x - prev->position_x) +
                    (current.position_y - prev->position_y) *
                    (current.position_y - prev->position_y);
    float dist_sq_thresh = duration * duration *
        max_stationary_speed_.val_ * max_stationary_speed_.val_;
    if (dist_sq > dist_sq_thresh)
      return false;
  }

  float dp_thresh = duration *
      (prev_result_suppress_finger_movement_ ?
       max_pressure_change_hysteresis_.val_ :
       max_pressure_change_.val_);
  float dp = fabsf(current.pressure - prev->pressure);
  return dp > dp_thresh;
}

bool ScrollManager::ComputeScroll(
    const HardwareStateBuffer& state_buffer,
    const FingerMap& prev_gs_fingers,
    const FingerMap& gs_fingers,
    GestureType prev_gesture_type,
    const Gesture& prev_result,
    Gesture* result,
    ScrollEventBuffer* scroll_buffer) {
  // For now, we take the movement of the biggest moving finger.
  float max_mag_sq = 0.0;  // square of max mag
  float dx = 0.0;
  float dy = 0.0;
  bool suppress_finger_movement = false;
  for (FingerMap::const_iterator it =
           gs_fingers.begin(), e = gs_fingers.end(); it != e; ++it) {
    const FingerState* fs = state_buffer.Get(0)->GetFingerState(*it);
    const FingerState* prev = state_buffer.Get(1)->GetFingerState(*it);
    if (!prev)
      return false;
    const stime_t dt =
        state_buffer.Get(0)->timestamp - state_buffer.Get(1)->timestamp;
    // Call SuppressStationaryFingerMovement even if suppress_finger_movement
    // is already true, b/c it records updates.
    suppress_finger_movement =
        SuppressStationaryFingerMovement(*fs, *prev, dt) ||
        suppress_finger_movement ||
        StationaryFingerPressureChangingSignificantly(state_buffer, *fs);
    float local_dx = fs->position_x - prev->position_x;
    if (fs->flags & GESTURES_FINGER_WARP_X_NON_MOVE)
      local_dx = 0.0;
    float local_dy = fs->position_y - prev->position_y;
    if (fs->flags & GESTURES_FINGER_WARP_Y_NON_MOVE)
      local_dy = 0.0;
    float local_max_mag_sq = local_dx * local_dx + local_dy * local_dy;
    if (local_max_mag_sq > max_mag_sq) {
      max_mag_sq = local_max_mag_sq;
      dx = local_dx;
      dy = local_dy;
    }
  }

  // See if we should snap to vertical/horizontal
  if (fabsf(dy) < horizontal_scroll_snap_slope_.val_ * fabsf(dx))
    dy = 0.0;  // snap to horizontal
  else if (fabsf(dy) > vertical_scroll_snap_slope_.val_ * fabsf(dx))
    dx = 0.0;  // snap to vertical

  prev_result_suppress_finger_movement_ = suppress_finger_movement;
  if (suppress_finger_movement) {
    // If we get here, it means that the pressure of the finger causing
    // the scroll is changing a lot, so we don't trust it. It's likely
    // leaving the touchpad. Normally we might just do nothing, but having
    // a frame or two of 0 length scroll before a fling looks janky. We
    // could also just start the fling now, but we don't want to do that
    // because the fingers may not actually be leaving. What seems to work
    // well is sort of dead-reckoning approach where we just repeat the
    // scroll event from the previous input frame.
    // Since this isn't a "real" scroll event, we don't put it into
    // scroll_buffer_.
    // Also, only use previous gesture if it's in the same direction.
    if (prev_result.type == kGestureTypeScroll &&
        prev_result.details.scroll.dy * dy >= 0 &&
        prev_result.details.scroll.dx * dx >= 0 &&
        (fabsf(prev_result.details.scroll.dy) >=
         min_scroll_dead_reckoning_.val_ ||
         fabsf(prev_result.details.scroll.dx) >=
         min_scroll_dead_reckoning_.val_)) {
      did_generate_scroll_ = true;
      *result = prev_result;
    } else if (min_scroll_dead_reckoning_.val_ > 0.0) {
      scroll_buffer->Clear();
    }
    return false;
  }

  if (max_mag_sq > 0) {
    did_generate_scroll_ = true;
    *result = Gesture(kGestureScroll,
                      state_buffer.Get(1)->timestamp,
                      state_buffer.Get(0)->timestamp,
                      dx, dy);
  }
  if (prev_gesture_type != kGestureTypeScroll || prev_gs_fingers != gs_fingers)
    scroll_buffer->Clear();
  if (!fling_buffer_suppress_zero_length_scrolls_.val_ ||
      !FloatEq(dx, 0.0) || !FloatEq(dy, 0.0))
    scroll_buffer->Insert(
        dx, dy,
        state_buffer.Get(0)->timestamp - state_buffer.Get(1)->timestamp);
  return true;
}

void ScrollManager::UpdateScrollEventBuffer(
    GestureType gesture_type, ScrollEventBuffer* scroll_buffer) const {
  if (gesture_type != kGestureTypeScroll)
    scroll_buffer->Clear();
}

size_t ScrollManager::ScrollEventsForFlingCount(
    const ScrollEventBuffer& scroll_buffer) const {
  if (scroll_buffer.Size() <= 1)
    return scroll_buffer.Size();
  enum Direction { kNone, kUp, kDown, kLeft, kRight };
  size_t i = 0;
  Direction prev_direction = kNone;
  size_t fling_buffer_depth = static_cast<size_t>(fling_buffer_depth_.val_);
  for (; i < scroll_buffer.Size() && i < fling_buffer_depth; i++) {
    const ScrollEvent& event = scroll_buffer.Get(i);
    if (FloatEq(event.dx, 0.0) && FloatEq(event.dy, 0.0))
      break;
    Direction direction;
    if (fabsf(event.dx) > fabsf(event.dy))
      direction = event.dx > 0 ? kRight : kLeft;
    else
      direction = event.dy > 0 ? kDown : kUp;
    if (i > 0 && direction != prev_direction)
      break;
    prev_direction = direction;
  }
  return i;
}

void ScrollManager::RegressScrollVelocity(
    const ScrollEventBuffer& scroll_buffer, int count, ScrollEvent* out) const {
  struct RegressionSums {
    float tt_;  // Cumulative sum of t^2.
    float t_;   // Cumulative sum of t.
    float tx_;  // Cumulative sum of t * x.
    float ty_;  // Cumulative sum of t * y.
    float x_;   // Cumulative sum of x.
    float y_;   // Cumulative sum of y.
  };

  out->dt = 1;
  if (count <= 1) {
    out->dx = 0;
    out->dy = 0;
    return;
  }

  RegressionSums sums = {0, 0, 0, 0, 0, 0};

  float time = 0;
  float x_coord = 0;
  float y_coord = 0;

  for (int i = count - 1; i >= 0; --i) {
    const ScrollEvent& event = scroll_buffer.Get(i);

    time += event.dt;
    x_coord += event.dx;
    y_coord += event.dy;

    sums.tt_ += time * time;
    sums.t_ += time;
    sums.tx_ += time * x_coord;
    sums.ty_ += time * y_coord;
    sums.x_ += x_coord;
    sums.y_ += y_coord;
  }

  // Note the regression determinant only depends on the values of t, and should
  // never be zero so long as (1) count > 1, and (2) dt values are all non-zero.
  float det = count * sums.tt_ - sums.t_ * sums.t_;

  if (det) {
    float det_inv = 1.0 / det;

    out->dx = (count * sums.tx_ - sums.t_ * sums.x_) * det_inv;
    out->dy = (count * sums.ty_ - sums.t_ * sums.y_) * det_inv;
  } else {
    out->dx = 0;
    out->dy = 0;
  }
}

bool ScrollManager::SuppressStationaryFingerMovement(const FingerState& fs,
                                                     const FingerState& prev,
                                                     stime_t dt) {
  if (max_stationary_move_speed_.val_ <= 0.0 ||
      max_stationary_move_suppress_distance_.val_ <= 0.0)
    return false;
  float dist_sq = DistSq(fs, prev);
  // If speed exceeded, allow free movement and discard history
  if (dist_sq > dt * dt *
      max_stationary_move_speed_.val_ * max_stationary_move_speed_.val_) {
    stationary_move_distance_.erase(fs.tracking_id);
    return false;
  }

  float dist = sqrtf(dist_sq);
  if (dist_sq <= dt * dt *
      max_stationary_move_speed_hysteresis_.val_ *
      max_stationary_move_speed_hysteresis_.val_ &&
      !MapContainsKey(stationary_move_distance_, fs.tracking_id)) {
    // We assume that the first nearly-stationay event won't exceed the
    // distance threshold and return from here.
    stationary_move_distance_[fs.tracking_id] = dist;
    return true;
  }

  if (!MapContainsKey(stationary_move_distance_, fs.tracking_id)) {
    return false;
  }

  // Check if distance exceeded. If so, keep history and allow motion
  stationary_move_distance_[fs.tracking_id] += dist;
  return stationary_move_distance_[fs.tracking_id] <
      max_stationary_move_suppress_distance_.val_;
}

void ScrollManager::ComputeFling(const HardwareStateBuffer& state_buffer,
                                 const ScrollEventBuffer& scroll_buffer,
                                 Gesture* result) const {
  if (!did_generate_scroll_)
    return;
  ScrollEvent out = { 0.0, 0.0, 0.0 };
  ScrollEvent zero = { 0.0, 0.0, 0.0 };
  size_t count = 0;

  // Make sure fling buffer met the minimum average speed for a fling.
  float buf_dist_sq = 0.0;
  float buf_dt = 0.0;
  scroll_buffer.GetSpeedSq(fling_buffer_depth_.val_, &buf_dist_sq, &buf_dt);
  if (fling_buffer_min_avg_speed_.val_ * fling_buffer_min_avg_speed_.val_ *
      buf_dt * buf_dt > buf_dist_sq) {
    out = zero;
    goto done;
  }

  count = ScrollEventsForFlingCount(scroll_buffer);
  if (count > scroll_buffer.Size()) {
    Err("Too few events in scroll buffer");
    out = zero;
    goto done;
  }

  if (count < 2) {
    if (count == 0)
      out = zero;
    else if (count == 1)
      out = scroll_buffer.Get(0);
    goto done;
  }

  // If we get here, count == 3 && scroll_buffer.Size() >= 3
  RegressScrollVelocity(scroll_buffer, count, &out);

done:
  float vx = out.dt ? (out.dx / out.dt) : 0.0;
  float vy = out.dt ? (out.dy / out.dt) : 0.0;
  *result = Gesture(kGestureFling,
                    state_buffer.Get(1)->timestamp,
                    state_buffer.Get(0)->timestamp,
                    vx,
                    vy,
                    GESTURES_FLING_START);
}

FingerButtonClick::FingerButtonClick(const ImmediateInterpreter* interpreter)
    : interpreter_(interpreter),
      fingers_(),
      fingers_status_(),
      num_fingers_(0),
      num_recent_(0),
      num_cold_(0),
      num_hot_(0) {
}

bool FingerButtonClick::Update(const HardwareState& hwstate,
                               stime_t button_down_time) {
  const float kMoveDistSq = interpreter_->button_move_dist_.val_ *
                            interpreter_->button_move_dist_.val_;

  // Copy all fingers to an array, but leave out palms
  num_fingers_ = 0;
  for (int i = 0; i < hwstate.touch_cnt; ++i) {
    const FingerState& fs = hwstate.fingers[i];
    if (fs.flags & (GESTURES_FINGER_PALM | GESTURES_FINGER_POSSIBLE_PALM))
      continue;
    // we don't support more than 4 fingers
    if (num_fingers_ >= 4)
      return false;
    fingers_[num_fingers_++] = &fs;
  }

  // Single finger is trivial
  if (num_fingers_ <= 1)
    return false;

  // Sort fingers array by origin timestamp
  FingerOriginCompare comparator(interpreter_);
  std::sort(fingers_, fingers_ + num_fingers_, comparator);

  // The status describes if a finger is recent (touched down recently),
  // cold (touched down a while ago, but did not move) or hot (has moved).
  // However thumbs are always forced to be "cold".
  for (int i = 0; i < num_fingers_; ++i) {
    stime_t finger_age =
        button_down_time -
        interpreter_->finger_origin_timestamp(fingers_[i]->tracking_id);
    bool moving_finger =
        SetContainsValue(interpreter_->moving_, fingers_[i]->tracking_id) ||
        (interpreter_->DistanceTravelledSq(*fingers_[i], true) > kMoveDistSq);
    if (!SetContainsValue(interpreter_->pointing_, fingers_[i]->tracking_id))
      fingers_status_[i] = STATUS_COLD;
    else if (moving_finger)
      fingers_status_[i] = STATUS_HOT;
    else if (finger_age < interpreter_->right_click_second_finger_age_.val_)
      fingers_status_[i] = STATUS_RECENT;
    else
      fingers_status_[i] = STATUS_COLD;
  }

  num_recent_ = 0;
  for (int i = 0; i < num_fingers_; ++i)
    num_recent_ += (fingers_status_[i] == STATUS_RECENT);

  num_cold_ = 0;
  for (int i = 0; i < num_fingers_; ++i)
    num_cold_ += (fingers_status_[i] == STATUS_COLD);

  num_hot_ = num_fingers_ - num_recent_ - num_cold_;
  return true;
}

int FingerButtonClick::GetButtonTypeForTouchCount(int touch_count) const {
  if (touch_count == 2)
    return GESTURES_BUTTON_RIGHT;
  if (touch_count == 3 && interpreter_->three_finger_click_enable_.val_)
    return GESTURES_BUTTON_MIDDLE;
  return GESTURES_BUTTON_LEFT;
}

int FingerButtonClick::EvaluateTwoFingerButtonType() {
  // Only one finger hot -> moving -> left click
  if (num_hot_ == 1)
    return GESTURES_BUTTON_LEFT;

  float start_delta =
      fabs(interpreter_->finger_origin_timestamp(fingers_[0]->tracking_id) -
           interpreter_->finger_origin_timestamp(fingers_[1]->tracking_id));

  // check if fingers are too close for a right click
  const float kMin2fDistThreshSq =
      interpreter_->tapping_finger_min_separation_.val_ *
      interpreter_->tapping_finger_min_separation_.val_;
  float dist_sq = DistSq(*fingers_[0], *fingers_[1]);
  if (dist_sq < kMin2fDistThreshSq)
    return GESTURES_BUTTON_LEFT;

  // fingers touched down at approx the same time
  if (start_delta < interpreter_->right_click_start_time_diff_.val_) {
    // If two fingers are both very recent, it could either be a right-click
    // or the left-click of one click-and-drag gesture. Our heuristic is that
    // for real right-clicks, two finger's pressure should be roughly the same
    // and they tend not be vertically aligned.
    const FingerState* min_fs = NULL;
    const FingerState* fs = NULL;
    if (fingers_[0]->pressure < fingers_[1]->pressure)
      min_fs = fingers_[0], fs = fingers_[1];
    else
      min_fs = fingers_[1], fs = fingers_[0];
    float min_pressure = min_fs->pressure;
    // It takes higher pressure for the bottom finger to trigger the physical
    // click and people tend to place fingers more vertically so that they have
    // enough space to drag the content with ease.
    bool likely_click_drag =
        (fs->pressure >
             min_pressure +
                 interpreter_->click_drag_pressure_diff_thresh_.val_ &&
         fs->pressure >
             min_pressure *
                 interpreter_->click_drag_pressure_diff_factor_.val_ &&
         fs->position_y > min_fs->position_y);
    float xdist = fabsf(fs->position_x - min_fs->position_x);
    float ydist = fabsf(fs->position_y - min_fs->position_y);
    if (likely_click_drag &&
        ydist >= xdist * interpreter_->click_drag_min_slope_.val_)
      return GESTURES_BUTTON_LEFT;
    return GESTURES_BUTTON_RIGHT;
  }

  // 1 finger is cold and in the dampened zone? Probably a thumb!
  if (num_cold_ == 1 && interpreter_->FingerInDampenedZone(*fingers_[0]))
    return GESTURES_BUTTON_LEFT;

  // Close fingers -> same hand -> right click
  // Fingers apart -> second hand finger or thumb -> left click
  if (interpreter_->TwoFingersGesturing(*fingers_[0], *fingers_[1], true))
    return GESTURES_BUTTON_RIGHT;
  else
    return GESTURES_BUTTON_LEFT;
}

int FingerButtonClick::EvaluateThreeOrMoreFingerButtonType() {
  // Treat recent, ambiguous fingers as thumbs if they are in the dampened
  // zone.
  int num_dampened_recent = 0;
  for (int i = num_fingers_ - num_recent_; i < num_fingers_; ++i)
    num_dampened_recent += interpreter_->FingerInDampenedZone(*fingers_[i]);

  // Re-use the 2f button type logic in case that all recent fingers are
  // presumed thumbs because the recent fingers could be from thumb splits
  // due to the increased pressure when doing a physical click and should be
  // ignored.
  if ((num_fingers_ - num_recent_ == 2) &&
      (num_recent_ == num_dampened_recent))
    return EvaluateTwoFingerButtonType();

  // Only one finger hot with all others cold -> moving -> left click
  if (num_hot_ == 1 && num_cold_ == num_fingers_ - 1)
    return GESTURES_BUTTON_LEFT;

  // A single recent touch, or a single cold touch (with all others being hot)
  // could be a thumb or a second hand finger.
  if (num_recent_ == 1 || (num_cold_ == 1 && num_hot_ == num_fingers_ - 1)) {
    // The ambiguous finger is either the most recent one, or the only cold one.
    const FingerState* ambiguous_finger = fingers_[num_fingers_ - 1];
    if (num_recent_ != 1) {
      for (int i = 0; i < num_fingers_; ++i) {
        if (fingers_status_[i] == STATUS_COLD) {
          ambiguous_finger = fingers_[i];
          break;
        }
      }
    }
    // If it's in the dampened zone we will expect it to be a thumb.
    // Otherwise it's a second hand finger
    if (interpreter_->FingerInDampenedZone(*ambiguous_finger))
      return GetButtonTypeForTouchCount(num_fingers_ - 1);
    else
      return GESTURES_BUTTON_LEFT;
  }

  // If all fingers are recent we can be sure they are from the same hand.
  if (num_recent_ == num_fingers_) {
    // Only if all fingers are in the same zone, we can be sure that none
    // of them is a thumb.
    Log("EvaluateThreeOrMoreFingerButtonType: Dampened: %d",
        num_dampened_recent);
    if (num_dampened_recent == 0 || num_dampened_recent == num_recent_)
      return GetButtonTypeForTouchCount(num_recent_);
  }

  // To make a decision after this point we need to figure out if and how
  // many of the fingers are grouped together. We do so by finding the pair
  // of closest fingers, and then calculate where we expect the remaining
  // fingers to be found.
  // If they are not in the expected place, they will be called separate.
  Log("EvaluateThreeOrMoreFingerButtonType: Falling back to location based "
      "detection");
  return EvaluateButtonTypeUsingFigureLocation();
}

int FingerButtonClick::EvaluateButtonTypeUsingFigureLocation() {
  const float kMaxDistSq = interpreter_->button_max_dist_from_expected_.val_ *
                           interpreter_->button_max_dist_from_expected_.val_;

  // Find pair with the closest distance
  const FingerState* pair_a = NULL;
  const FingerState* pair_b = NULL;
  float pair_dist_sq = std::numeric_limits<float>::infinity();
  for (int i = 0; i < num_fingers_; ++i) {
    for (int j = 0; j < i; ++j) {
      float dist_sq = DistSq(*fingers_[i], *fingers_[j]);
      if (dist_sq < pair_dist_sq) {
        pair_a = fingers_[i];
        pair_b = fingers_[j];
        pair_dist_sq = dist_sq;
      }
    }
  }

  int num_separate = 0;
  const FingerState* last_separate = NULL;

  if (interpreter_->metrics_->CloseEnoughToGesture(Vector2(*pair_a),
                                                   Vector2(*pair_b))) {
    // Expect the remaining fingers to be next to the pair, all with the same
    // distance from each other.
    float dx = pair_b->position_x - pair_a->position_x;
    float dy = pair_b->position_y - pair_a->position_y;
    float expected1_x = pair_a->position_x + 2 * dx;
    float expected1_y = pair_a->position_y + 2 * dy;
    float expected2_x = pair_b->position_x - 2 * dx;
    float expected2_y = pair_b->position_y - 2 * dy;

    // Check if remaining fingers are close to the expected positions
    for (int i = 0; i < num_fingers_; ++i) {
      if (fingers_[i] == pair_a || fingers_[i] == pair_b)
        continue;
      float dist1_sq = DistSqXY(*fingers_[i], expected1_x, expected1_y);
      float dist2_sq = DistSqXY(*fingers_[i], expected2_x, expected2_y);
      if (dist1_sq > kMaxDistSq && dist2_sq > kMaxDistSq) {
        num_separate++;
        last_separate = fingers_[i];
      }
    }
  } else {
    // In case the pair is not close we have to fall back to using the
    // dampened zone
    Log("EvaluateButtonTypeUsingFigureLocation: Falling back to dampened zone "
        "separation");
    for (int i = 0; i < num_fingers_; ++i) {
      if (interpreter_->FingerInDampenedZone(*fingers_[i])) {
        num_separate++;
        last_separate = fingers_[i];
      }
    }
  }

  // All fingers next to each other
  if (num_separate == 0)
    return GetButtonTypeForTouchCount(num_fingers_);

  // The group with the last finger counts!
  // Exception: If the separates have only one finger and it's a thumb
  //            count the other group
  int num_pressing;
  if (fingers_[num_fingers_ - 1] == last_separate &&
      !(num_separate == 1 &&
        interpreter_->FingerInDampenedZone(*last_separate))) {
    num_pressing = num_separate;
  } else {
    num_pressing = num_fingers_ - num_separate;
  }
  Log("EvaluateButtonTypeUsingFigureLocation: Pressing: %d", num_pressing);
  return GetButtonTypeForTouchCount(num_pressing);
}

ImmediateInterpreter::ImmediateInterpreter(PropRegistry* prop_reg,
                                           Tracer* tracer)
    : Interpreter(NULL, tracer, false),
      button_type_(0),
      finger_button_click_(this),
      sent_button_down_(false),
      button_down_timeout_(0.0),
      started_moving_time_(-1.0),
      gs_changed_time_(-1.0),
      finger_leave_time_(0.0),
      moving_finger_id_(-1),
      tap_to_click_state_(kTtcIdle),
      tap_to_click_state_entered_(0.0),
      tap_record_(this),
      last_movement_timestamp_(0.0),
      last_swipe_timestamp_(0.0),
      swipe_is_vertical_(false),
      current_gesture_type_(kGestureTypeNull),
      state_buffer_(8),
      scroll_buffer_(20),
      these_fingers_scrolled_(false),
      pinch_guess_start_(-1.0),
      pinch_locked_(false),
      pinch_status_(GESTURES_ZOOM_START),
      pinch_prev_direction_(0),
      pinch_prev_time_(-1.0),
      finger_seen_shortly_after_button_down_(false),
      scroll_manager_(prop_reg),
      tap_enable_(prop_reg, "Tap Enable", true),
      tap_paused_(prop_reg, "Tap Paused", false),
      tap_timeout_(prop_reg, "Tap Timeout", 0.2),
      inter_tap_timeout_(prop_reg, "Inter-Tap Timeout", 0.15),
      tap_drag_delay_(prop_reg, "Tap Drag Delay", 0.05),
      tap_drag_timeout_(prop_reg, "Tap Drag Timeout", 0.3),
      tap_drag_enable_(prop_reg, "Tap Drag Enable", 0),
      drag_lock_enable_(prop_reg, "Tap Drag Lock Enable", 0),
      tap_drag_stationary_time_(prop_reg, "Tap Drag Stationary Time", 0),
      tap_move_dist_(prop_reg, "Tap Move Distance", 2.0),
      tap_min_pressure_(prop_reg, "Tap Minimum Pressure", 25.0),
      tap_max_movement_(prop_reg, "Tap Maximum Movement", 0.0001),
      tap_max_finger_age_(prop_reg, "Tap Maximum Finger Age", 1.2),
      three_finger_click_enable_(prop_reg, "Three Finger Click Enable", 1),
      zero_finger_click_enable_(prop_reg, "Zero Finger Click Enable", 1),
      t5r2_three_finger_click_enable_(prop_reg,
                                      "T5R2 Three Finger Click Enable",
                                      0),
      change_move_distance_(prop_reg, "Change Min Move Distance", 3.0),
      move_lock_speed_(prop_reg, "Move Lock Speed", 10.0),
      move_report_distance_(prop_reg, "Move Report Distance", 0.35),
      change_timeout_(prop_reg, "Change Timeout", 0.04),
      evaluation_timeout_(prop_reg, "Evaluation Timeout", 0.15),
      pinch_evaluation_timeout_(prop_reg, "Pinch Evaluation Timeout", 0.1),
      thumb_pinch_evaluation_timeout_(prop_reg,
                                      "Thumb Pinch Evaluation Timeout", 0.25),
      thumb_pinch_min_movement_(prop_reg,
                                "Thumb Pinch Minimum Movement", 0.8),
      slow_pinch_guess_ratio_threshold_(prop_reg,
                                "Slow Pinch Guess Ratio Threshold", 0.8),
      thumb_pinch_movement_ratio_(prop_reg, "Thumb Pinch Movement Ratio", 20),
      thumb_slow_pinch_similarity_ratio_(prop_reg,
                                         "Thumb Slow Pinch Similarity Ratio",
                                         5),
      thumb_pinch_delay_factor_(prop_reg, "Thumb Pinch Delay Factor", 9.0),
      minimum_movement_direction_detection_(prop_reg,
          "Minimum Movement Direction Detection", 0.003),
      damp_scroll_min_movement_factor_(prop_reg,
                                       "Damp Scroll Min Move Factor",
                                       0.2),
      two_finger_pressure_diff_thresh_(prop_reg,
                                       "Two Finger Pressure Diff Thresh",
                                       32.0),
      two_finger_pressure_diff_factor_(prop_reg,
                                       "Two Finger Pressure Diff Factor",
                                       1.65),
      click_drag_pressure_diff_thresh_(prop_reg,
                                       "Click Drag Pressure Diff Thresh",
                                       10.0),
      click_drag_pressure_diff_factor_(prop_reg,
                                       "Click Drag Pressure Diff Factor",
                                       1.20),
      click_drag_min_slope_(prop_reg, "Click Drag Min Slope", 2.22),
      thumb_movement_factor_(prop_reg, "Thumb Movement Factor", 0.5),
      thumb_speed_factor_(prop_reg, "Thumb Speed Factor", 0.5),
      thumb_eval_timeout_(prop_reg, "Thumb Evaluation Timeout", 0.06),
      thumb_pinch_threshold_ratio_(prop_reg,
                                   "Thumb Pinch Threshold Ratio", 0.25),
      thumb_click_prevention_timeout_(prop_reg,
                                      "Thumb Click Prevention Timeout", 0.15),
      two_finger_scroll_distance_thresh_(prop_reg,
                                         "Two Finger Scroll Distance Thresh",
                                         1.5),
      two_finger_move_distance_thresh_(prop_reg,
                                       "Two Finger Move Distance Thresh",
                                       7.0),
      three_finger_close_distance_thresh_(prop_reg,
                                          "Three Finger Close Distance Thresh",
                                          55.0),
      four_finger_close_distance_thresh_(prop_reg,
                                         "Four Finger Close Distance Thresh",
                                         60.0),
      three_finger_swipe_distance_thresh_(prop_reg,
                                          "Three Finger Swipe Distance Thresh",
                                          1.5),
      four_finger_swipe_distance_thresh_(prop_reg,
                                         "Four Finger Swipe Distance Thresh",
                                         1.5),
      three_finger_swipe_distance_ratio_(prop_reg,
                                          "Three Finger Swipe Distance Ratio",
                                          0.2),
      four_finger_swipe_distance_ratio_(prop_reg,
                                         "Four Finger Swipe Distance Ratio",
                                         0.2),
      three_finger_swipe_enable_(prop_reg, "Three Finger Swipe EnableX", 1),
      bottom_zone_size_(prop_reg, "Bottom Zone Size", 10.0),
      button_evaluation_timeout_(prop_reg, "Button Evaluation Timeout", 0.05),
      button_finger_timeout_(prop_reg, "Button Finger Timeout", 0.03),
      button_move_dist_(prop_reg, "Button Move Distance", 10.0),
      button_max_dist_from_expected_(prop_reg,
                                     "Button Max Distance From Expected", 20.0),
      button_right_click_zone_enable_(prop_reg,
                                      "Button Right Click Zone Enable", 0),
      button_right_click_zone_size_(prop_reg,
                                    "Button Right Click Zone Size", 20.0),
      keyboard_touched_timeval_high_(prop_reg, "Keyboard Touched Timeval High",
                                     0),
      keyboard_touched_timeval_low_(prop_reg, "Keyboard Touched Timeval Low",
                                    0, this),
      keyboard_touched_(0.0),
      keyboard_palm_prevent_timeout_(prop_reg, "Keyboard Palm Prevent Timeout",
                                     0.5),
      motion_tap_prevent_timeout_(prop_reg, "Motion Tap Prevent Timeout",
                                  0.05),
      tapping_finger_min_separation_(prop_reg, "Tap Min Separation", 10.0),
      no_pinch_guess_ratio_(prop_reg, "No-Pinch Guess Ratio", 0.9),
      no_pinch_certain_ratio_(prop_reg, "No-Pinch Certain Ratio", 2.0),
      pinch_noise_level_sq_(prop_reg, "Pinch Noise Level Squared", 2.0),
      pinch_guess_min_movement_(prop_reg, "Pinch Guess Minimum Movement", 2.0),
      pinch_thumb_min_movement_(prop_reg,
                                "Pinch Thumb Minimum Movement", 1.41),
      pinch_certain_min_movement_(prop_reg,
                                  "Pinch Certain Minimum Movement", 8.0),
      inward_pinch_min_angle_(prop_reg, "Inward Pinch Minimum Angle", 0.3),
      pinch_zoom_max_angle_(prop_reg, "Pinch Zoom Maximum Angle", -0.4),
      scroll_min_angle_(prop_reg, "Scroll Minimum Angle", -0.2),
      pinch_guess_consistent_mov_ratio_(prop_reg,
          "Pinch Guess Consistent Movement Ratio", 0.4),
      pinch_zoom_min_events_(prop_reg, "Pinch Zoom Minimum Events", 3),
      pinch_initial_scale_time_inv_(prop_reg,
                                    "Pinch Initial Scale Time Inverse",
                                    3.33),
      pinch_res_(prop_reg, "Minimum Pinch Scale Resolution Squared", 1.005),
      pinch_stationary_res_(prop_reg,
                            "Stationary Pinch Scale Resolution Squared",
                            1.05),
      pinch_stationary_time_(prop_reg,
                             "Stationary Pinch Time",
                             0.10),
      pinch_hysteresis_res_(prop_reg,
                            "Hysteresis Pinch Scale Resolution Squared",
                            1.05),
      pinch_enable_(prop_reg, "Pinch Enable", 0),
      right_click_start_time_diff_(prop_reg,
                                   "Right Click Start Time Diff Thresh",
                                   0.1),
      right_click_second_finger_age_(prop_reg,
                                     "Right Click Second Finger Age Thresh",
                                     0.5),
      quick_acceleration_factor_(prop_reg, "Quick Acceleration Factor", 0.0) {
  InitName();
  requires_metrics_ = true;
}

void ImmediateInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                             stime_t* timeout) {
  if (!state_buffer_.Get(0)->fingers) {
    Err("Must call SetHardwareProperties() before Push().");
    return;
  }

  state_buffer_.PushState(*hwstate);

  FillOriginInfo(*hwstate);
  result_.type = kGestureTypeNull;
  const bool same_fingers = state_buffer_.Get(1)->SameFingersAs(*hwstate) &&
      (hwstate->buttons_down == state_buffer_.Get(1)->buttons_down);
  if (!same_fingers) {
    // Fingers changed, do nothing this time
    FingerMap new_gs_fingers =
        SetSubtract(GetGesturingFingers(*hwstate), non_gs_fingers_);
    ResetSameFingersState(*hwstate);
    FillStartPositions(*hwstate);
    if (pinch_enable_.val_ &&
        (hwstate->finger_cnt <= 2 || new_gs_fingers.size() != 2)) {
      // Release the zoom lock
      UpdatePinchState(*hwstate, true, new_gs_fingers);
    }
    moving_finger_id_ = -1;
  }

  if (hwstate->finger_cnt < state_buffer_.Get(1)->finger_cnt)
    finger_leave_time_ = hwstate->timestamp;

  UpdatePointingFingers(*hwstate);
  UpdateThumbState(*hwstate);
  FingerMap newly_moving_fingers = UpdateMovingFingers(*hwstate);
  UpdateNonGsFingers(*hwstate);
  FingerMap gs_fingers =
      SetSubtract(GetGesturingFingers(*hwstate), non_gs_fingers_);
  if (gs_fingers != prev_gs_fingers_)
    gs_changed_time_ = hwstate->timestamp;
  UpdateStartedMovingTime(hwstate->timestamp, gs_fingers, newly_moving_fingers);

  UpdateButtons(*hwstate, timeout);
  UpdateTapGesture(hwstate,
                   gs_fingers,
                   same_fingers,
                   hwstate->timestamp,
                   timeout);

  FingerMap active_gs_fingers;
  UpdateCurrentGestureType(*hwstate, gs_fingers, &active_gs_fingers);
  non_gs_fingers_ = SetSubtract(gs_fingers, active_gs_fingers);
  if (result_.type == kGestureTypeNull)
    FillResultGesture(*hwstate, active_gs_fingers);

  // Prevent moves while in a tap
  if ((tap_to_click_state_ == kTtcFirstTapBegan ||
       tap_to_click_state_ == kTtcSubsequentTapBegan) &&
      result_.type == kGestureTypeMove)
    result_.type = kGestureTypeNull;

  prev_active_gs_fingers_ = active_gs_fingers;
  prev_gs_fingers_ = gs_fingers;
  prev_result_ = result_;
  prev_gesture_type_ = current_gesture_type_;
  if (result_.type != kGestureTypeNull)
    ProduceGesture(result_);
}

void ImmediateInterpreter::HandleTimerImpl(stime_t now, stime_t* timeout) {
  result_.type = kGestureTypeNull;
  // Tap-to-click always aborts when real button(s) are being used, so we
  // don't need to worry about conflicts with these two types of callback.
  UpdateButtonsTimeout(now);
  UpdateTapGesture(NULL,
                   FingerMap(),
                   false,
                   now,
                   timeout);
  if (result_.type != kGestureTypeNull)
    ProduceGesture(result_);
}

void ImmediateInterpreter::FillOriginInfo(
    const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&origin_timestamps_, hwstate);
  RemoveMissingIdsFromMap(&distance_walked_, hwstate);
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (MapContainsKey(origin_timestamps_, fs.tracking_id) &&
        state_buffer_.Size() > 1 &&
        state_buffer_.Get(1)->GetFingerState(fs.tracking_id)) {
      float delta_x = hwstate.GetFingerState(fs.tracking_id)->position_x -
          state_buffer_.Get(1)->GetFingerState(fs.tracking_id)->position_x;
      float delta_y = hwstate.GetFingerState(fs.tracking_id)->position_y -
          state_buffer_.Get(1)->GetFingerState(fs.tracking_id)->position_y;
      distance_walked_[fs.tracking_id] += sqrtf(delta_x * delta_x +
                                                delta_y * delta_y);
      continue;
    }
    origin_timestamps_[fs.tracking_id] = hwstate.timestamp;
    distance_walked_[fs.tracking_id] = 0.0;
  }
}

void ImmediateInterpreter::ResetSameFingersState(const HardwareState& hwstate) {
  pointing_.clear();
  fingers_.clear();
  start_positions_.clear();
  scroll_manager_.ResetSameFingerState();
  RemoveMissingIdsFromSet(&moving_, hwstate);
  changed_time_ = hwstate.timestamp;
}

void ImmediateInterpreter::UpdatePointingFingers(const HardwareState& hwstate) {
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    if (hwstate.fingers[i].flags & GESTURES_FINGER_PALM)
      pointing_.erase(hwstate.fingers[i].tracking_id);
    else
      pointing_.insert(hwstate.fingers[i].tracking_id);
  }
  fingers_ = pointing_;
}

float ImmediateInterpreter::DistanceTravelledSq(const FingerState& fs,
                                                bool origin,
                                                bool permit_warp) const {
  Point delta = FingerTraveledVector(fs, origin, permit_warp);
  return delta.x_ * delta.x_ + delta.y_ * delta.y_;
}

ImmediateInterpreter::Point ImmediateInterpreter::FingerTraveledVector(
    const FingerState& fs, bool origin, bool permit_warp) const {
  const map<short, Point, kMaxFingers>* positions;
  if (origin)
    positions = &origin_positions_;
  else
    positions = &start_positions_;

  if (!MapContainsKey(*positions, fs.tracking_id))
    return Point(0.0f, 0.0f);

  const Point& start = (*positions)[fs.tracking_id];
  float dx = fs.position_x - start.x_;
  float dy = fs.position_y - start.y_;
  bool suppress_move =
      (!permit_warp || (fs.flags & GESTURES_FINGER_WARP_TELEPORTATION));
  if ((fs.flags & GESTURES_FINGER_WARP_X) && suppress_move)
    dx = 0;
  if ((fs.flags & GESTURES_FINGER_WARP_Y) && suppress_move)
    dy = 0;
  return Point(dx, dy);
}

bool ImmediateInterpreter::EarlyZoomPotential(const HardwareState& hwstate)
    const {
  if (fingers_.size() != 2)
    return false;
  int id1 = *(fingers_.begin());
  int id2 = *(fingers_.begin() + 1);
  const FingerState* finger1 = hwstate.GetFingerState(id1);
  const FingerState* finger2 = hwstate.GetFingerState(id2);
  float pinch_eval_timeout = pinch_evaluation_timeout_.val_;
  if (finger1 == NULL || finger2 == NULL)
    return false;
  // Wait for a longer time if fingers arrived together
  if (fabs(origin_timestamps_[id1] - origin_timestamps_[id2]) <
          evaluation_timeout_.val_ &&
      hwstate.timestamp -
          max(origin_timestamps_[id1], origin_timestamps_[id2]) <
          thumb_pinch_evaluation_timeout_.val_ * thumb_pinch_delay_factor_.val_)
    pinch_eval_timeout *= thumb_pinch_delay_factor_.val_;
  bool early_decision = hwstate.timestamp -
                        min(origin_timestamps_[finger1->tracking_id],
                            origin_timestamps_[finger2->tracking_id]) <
                        pinch_eval_timeout;
  // Avoid extra computation if it's too late for a pinch zoom
  if (!early_decision &&
      hwstate.timestamp - origin_timestamps_[finger1->tracking_id] >
          thumb_pinch_evaluation_timeout_.val_)
    return false;

  float walked_distance1 = distance_walked_[finger1->tracking_id];
  float walked_distance2 = distance_walked_[finger2->tracking_id];
  if (walked_distance1 > walked_distance2)
    std::swap(walked_distance1,walked_distance2);
  if ((walked_distance1 > thumb_pinch_min_movement_.val_ ||
           hwstate.timestamp - origin_timestamps_[finger1->tracking_id] >
               thumb_pinch_evaluation_timeout_.val_) &&
      walked_distance1 > 0 &&
      walked_distance2 / walked_distance1 > thumb_pinch_movement_ratio_.val_)
    return false;

  bool motionless_cycles = false;
  for (int i = 1;
       i < min<int>(state_buffer_.Size(), pinch_zoom_min_events_.val_); i++) {
    const FingerState* curr1 = state_buffer_.Get(i - 1)->GetFingerState(id1);
    const FingerState* curr2 = state_buffer_.Get(i - 1)->GetFingerState(id2);
    const FingerState* prev1 = state_buffer_.Get(i)->GetFingerState(id1);
    const FingerState* prev2 = state_buffer_.Get(i)->GetFingerState(id2);
    if (!curr1 || !curr2 || !prev1 || !prev2) {
       motionless_cycles = true;
       break;
    }
    bool finger1_moved = (curr1->position_x - prev1->position_x) != 0 ||
                         (curr1->position_y - prev1->position_y) != 0;
    bool finger2_moved = (curr2->position_x - prev2->position_x) != 0 ||
                         (curr2->position_y - prev2->position_y) != 0;
    if (!finger1_moved && !finger2_moved) {
       motionless_cycles = true;
       break;
    }
  }
  if (motionless_cycles > 0 && early_decision)
    return true;

  Point delta1 = FingerTraveledVector(*finger1, true, true);
  Point delta2 = FingerTraveledVector(*finger2, true, true);
  float dot = delta1.x_ * delta2.x_ + delta1.y_ * delta2.y_;
  if ((pinch_guess_start_ > 0 || dot < 0) && early_decision)
    return true;

  if (origin_timestamps_[finger1->tracking_id] -
          origin_timestamps_[finger2->tracking_id] < evaluation_timeout_.val_ &&
      origin_timestamps_[finger2->tracking_id] -
          origin_timestamps_[finger1->tracking_id] < evaluation_timeout_.val_ &&
      hwstate.timestamp - origin_timestamps_[finger1->tracking_id] <
          thumb_pinch_evaluation_timeout_.val_)
    return true;

  return false;
}

bool ImmediateInterpreter::ZoomFingersAreConsistent(
    const HardwareStateBuffer& state_buffer) const {
  if (fingers_.size() != 2)
    return false;

  int id1 = *(fingers_.begin());
  int id2 = *(fingers_.begin() + 1);

  const FingerState* curr1 = state_buffer.Get(min<int>(state_buffer.Size() - 1,
      pinch_zoom_min_events_.val_))->GetFingerState(id1);
  const FingerState* curr2 = state_buffer.Get(min<int>(state_buffer.Size() - 1,
      pinch_zoom_min_events_.val_))->GetFingerState(id2);
  if (!curr1 || !curr2)
    return false;
  for (int i = 0;
       i < min<int>(state_buffer.Size(), pinch_zoom_min_events_.val_); i++) {
    const FingerState* prev1 = state_buffer.Get(i)->GetFingerState(id1);
    const FingerState* prev2 = state_buffer.Get(i)->GetFingerState(id2);
    if (!prev1 || !prev2)
      return false;
    float dot = FingersAngle(prev1, prev2, curr1, curr2);
    if (dot >= 0)
      return false;
  }
  const FingerState* last1 = state_buffer.Get(0)->GetFingerState(id1);
  const FingerState* last2 = state_buffer.Get(0)->GetFingerState(id2);
  float angle = FingersAngle(last1, last2, curr1, curr2);
  if (angle > pinch_zoom_max_angle_.val_)
    return false;
  return true;
}

bool ImmediateInterpreter::InwardPinch(
    const HardwareStateBuffer& state_buffer, const FingerState& fs) const {
  if (fingers_.size() != 2)
    return false;

  int id = fs.tracking_id;

  const FingerState* curr =
      state_buffer.Get(min<int>(state_buffer.Size(),
          pinch_zoom_min_events_.val_))->GetFingerState(id);
  if (!curr)
    return false;
  for (int i = 0;
       i < min<int>(state_buffer.Size(), pinch_zoom_min_events_.val_); i++) {
    const FingerState* prev = state_buffer.Get(i)->GetFingerState(id);
    if (!prev)
      return false;
    float dot = (curr->position_y - prev->position_y);
    if (dot <= 0)
      return false;
  }
  const FingerState* last = state_buffer.Get(0)->GetFingerState(id);
  float dot_last = (curr->position_y - last->position_y);
  float size_last = sqrt((curr->position_x - last->position_x) *
                         (curr->position_x - last->position_x) +
                         (curr->position_y - last->position_y) *
                         (curr->position_y - last->position_y));

  float angle = dot_last / size_last;
  if (angle < inward_pinch_min_angle_.val_)
    return false;
  return true;
}

float ImmediateInterpreter::FingersAngle(const FingerState* prev1,
                                         const FingerState* prev2,
                                         const FingerState* curr1,
                                         const FingerState* curr2) const {
  float dot_last = (curr1->position_x - prev1->position_x) *
                   (curr2->position_x - prev2->position_x) +
                   (curr1->position_y - prev1->position_y) *
                   (curr2->position_y - prev2->position_y);
  float size_last1_sq = (curr1->position_x - prev1->position_x) *
                        (curr1->position_x - prev1->position_x) +
                        (curr1->position_y - prev1->position_y) *
                        (curr1->position_y - prev1->position_y);
  float size_last2_sq = (curr2->position_x - prev2->position_x) *
                        (curr2->position_x - prev2->position_x) +
                        (curr2->position_y - prev2->position_y) *
                        (curr2->position_y - prev2->position_y);
  float overall_size = sqrt(size_last1_sq * size_last2_sq);
  // If one of the two vectors is too small, return 0.
  if (overall_size < minimum_movement_direction_detection_.val_ *
                     minimum_movement_direction_detection_.val_)
    return 0.0;
  return dot_last / overall_size;
}

bool ImmediateInterpreter::ScrollAngle(const FingerState& finger1,
                                       const FingerState& finger2) {
    const FingerState* curr1 = state_buffer_.Get(
        min<int>(state_buffer_.Size() - 1, 3))->
            GetFingerState(finger1.tracking_id);
    const FingerState* curr2 = state_buffer_.Get(
        min<int>(state_buffer_.Size() - 1, 3))->
            GetFingerState(finger2.tracking_id);
    const FingerState* last1 =
        state_buffer_.Get(0)->GetFingerState(finger1.tracking_id);
    const FingerState* last2 =
        state_buffer_.Get(0)->GetFingerState(finger2.tracking_id);
    if (last1 && last2 && curr1 && curr2) {
      if (FingersAngle(last1, last2, curr1, curr2) < scroll_min_angle_.val_)
        return false;
    }
    return true;
}

float ImmediateInterpreter::TwoFingerDistanceSq(
    const HardwareState& hwstate) const {
  if (fingers_.size() == 2) {
    return TwoSpecificFingerDistanceSq(hwstate, fingers_);
  } else {
    return -1;
  }
}

float ImmediateInterpreter::TwoSpecificFingerDistanceSq(
    const HardwareState& hwstate, const FingerMap& fingers) const {
  if (fingers.size() == 2) {
    const FingerState* finger_a = hwstate.GetFingerState(*fingers.begin());
    const FingerState* finger_b = hwstate.GetFingerState(
        *(fingers.begin() + 1));
    if (finger_a == NULL || finger_b == NULL) {
      Err("Finger unexpectedly NULL");
      return -1;
    }
    return DistSq(*finger_a, *finger_b);
  } else if (hwstate.finger_cnt == 2) {
    return DistSq(hwstate.fingers[0], hwstate.fingers[1]);
  } else {
    return -1;
  }
}

// Updates thumb_ below.
void ImmediateInterpreter::UpdateThumbState(const HardwareState& hwstate) {
  // Remove old ids from thumb_
  RemoveMissingIdsFromMap(&thumb_, hwstate);
  RemoveMissingIdsFromMap(&thumb_eval_timer_, hwstate);
  float min_pressure = INFINITY;
  const FingerState* min_fs = NULL;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (fs.flags & GESTURES_FINGER_PALM)
      continue;
    if (fs.pressure < min_pressure) {
      min_pressure = fs.pressure;
      min_fs = &fs;
    }
  }
  if (!min_fs) {
    // Only palms on the touchpad
    return;
  }
  // We respect warp flags only if we really have little information of the
  // finger positions and not just because we want to suppress unintentional
  // cursor moves. See the definition of GESTURES_FINGER_WARP_TELEPORTATION
  // for more detail.
  bool min_warp_move = (min_fs->flags & GESTURES_FINGER_WARP_TELEPORTATION) &&
                       ((min_fs->flags & GESTURES_FINGER_WARP_X_MOVE) ||
                        (min_fs->flags & GESTURES_FINGER_WARP_Y_MOVE));
  float min_dist_sq = DistanceTravelledSq(*min_fs, true, true);
  float min_dt = hwstate.timestamp -
      origin_timestamps_[min_fs->tracking_id];
  float thumb_dist_sq_thresh = min_dist_sq *
      thumb_movement_factor_.val_ * thumb_movement_factor_.val_;
  float thumb_speed_sq_thresh = min_dist_sq *
      thumb_speed_factor_.val_ * thumb_speed_factor_.val_;
  // Make all large-pressure, less moving contacts located below the
  // min-pressure contact as thumbs.
  bool similar_movement = false;

  if (pinch_enable_.val_ && hwstate.finger_cnt == 2) {
    float dt1 = hwstate.timestamp -
                origin_timestamps_[hwstate.fingers[0].tracking_id];
    float dist_sq1 = DistanceTravelledSq(hwstate.fingers[0], true, true);
    float dt2 = hwstate.timestamp -
                origin_timestamps_[hwstate.fingers[1].tracking_id];
    float dist_sq2 = DistanceTravelledSq(hwstate.fingers[1], true, true);
    if (dist_sq1 * dt1 && dist_sq2 * dt2)
      similar_movement = max((dist_sq1 * dt1 * dt1) / (dist_sq2 * dt2 * dt2),
                             (dist_sq2 * dt2 * dt2) / (dist_sq1 * dt1 * dt1)) <
                         thumb_slow_pinch_similarity_ratio_.val_;
    else
      similar_movement = false;
  }
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (fs.flags & GESTURES_FINGER_PALM)
      continue;
    if (pinch_enable_.val_ && InwardPinch(state_buffer_, fs)) {
      thumb_speed_sq_thresh *= thumb_pinch_threshold_ratio_.val_;
      thumb_dist_sq_thresh *= thumb_pinch_threshold_ratio_.val_;
    }
    float dist_sq = DistanceTravelledSq(fs, true, true);
    float dt = hwstate.timestamp - origin_timestamps_[fs.tracking_id];
    bool closer_to_origin = dist_sq <= thumb_dist_sq_thresh;
    bool slower_moved = (dist_sq * min_dt &&
                         dist_sq * min_dt * min_dt <
                         thumb_speed_sq_thresh * dt * dt);
    bool relatively_motionless = closer_to_origin || slower_moved;
    bool likely_thumb =
        (fs.pressure > min_pressure + two_finger_pressure_diff_thresh_.val_ &&
         fs.pressure > min_pressure * two_finger_pressure_diff_factor_.val_ &&
         fs.position_y > min_fs->position_y);
    bool non_gs = (hwstate.timestamp > changed_time_ &&
                   (prev_active_gs_fingers_.find(fs.tracking_id) ==
                    prev_active_gs_fingers_.end()) &&
                   prev_result_.type != kGestureTypeNull);
    non_gs |= moving_finger_id_ >= 0 && moving_finger_id_ != fs.tracking_id;
    likely_thumb |= non_gs;
    // We sometimes can't decide the thumb state if some fingers are undergoing
    // warp moves as the decision could be off (DistanceTravelledSq may
    // under-estimate the real distance). The cases that we need to re-evaluate
    // the thumb in the next frame are:
    // 1. Both fingers warp.
    // 2. Min-pressure finger warps and relatively_motionless is false.
    // 3. Thumb warps and relatively_motionless is true.
    bool warp_move = (fs.flags & GESTURES_FINGER_WARP_TELEPORTATION) &&
                     ((fs.flags & GESTURES_FINGER_WARP_X_MOVE) ||
                      (fs.flags & GESTURES_FINGER_WARP_Y_MOVE));
    if (likely_thumb &&
        ((warp_move && min_warp_move) ||
         (!warp_move && min_warp_move && !relatively_motionless) ||
         (warp_move && !min_warp_move && relatively_motionless))) {
      continue;
    }
    likely_thumb &= relatively_motionless;
    if (MapContainsKey(thumb_, fs.tracking_id)) {
      // Beyond the evaluation period. Stick to being thumbs.
      if (thumb_eval_timer_[fs.tracking_id] <= 0.0) {
        if (!pinch_enable_.val_ || hwstate.finger_cnt == 1)
          continue;
        bool slow_pinch_guess =
            dist_sq * min_dt * min_dt / (thumb_speed_sq_thresh * dt * dt) >
                thumb_pinch_min_movement_.val_ &&
            similar_movement;
        bool might_be_pinch = (slow_pinch_guess &&
                               hwstate.timestamp -
                                   origin_timestamps_[fs.tracking_id] < 2 *
                                   thumb_pinch_evaluation_timeout_.val_ &&
                               ZoomFingersAreConsistent(state_buffer_));
        if (relatively_motionless ||
            hwstate.timestamp - origin_timestamps_[fs.tracking_id] >
                thumb_pinch_evaluation_timeout_.val_) {
          if (!might_be_pinch)
            continue;
          else
            likely_thumb = false;
        }
      }

      // Finger is still under evaluation.
      if (likely_thumb) {
        // Decrease the timer as the finger is thumb-like in the previous
        // frame.
        const FingerState* prev =
            state_buffer_.Get(1)->GetFingerState(fs.tracking_id);
        if (!prev)
          continue;
        thumb_eval_timer_[fs.tracking_id] -=
            hwstate.timestamp - state_buffer_.Get(1)->timestamp;
      } else {
        // The finger wasn't thumb-like in the frame. Remove it from the thumb
        // list.
        thumb_.erase(fs.tracking_id);
        thumb_eval_timer_.erase(fs.tracking_id);
      }
    } else if (likely_thumb) {
      // Finger is thumb-like, so we add it to the list.
      thumb_[fs.tracking_id] = hwstate.timestamp;
      thumb_eval_timer_[fs.tracking_id] = thumb_eval_timeout_.val_;
    }
  }
  for (map<short, stime_t, kMaxFingers>::const_iterator it = thumb_.begin();
       it != thumb_.end(); ++it)
    pointing_.erase((*it).first);
}

void ImmediateInterpreter::UpdateNonGsFingers(const HardwareState& hwstate) {
  RemoveMissingIdsFromSet(&non_gs_fingers_, hwstate);
  // moving fingers may be gesturing, so take them out from the set.
  non_gs_fingers_ = SetSubtract(non_gs_fingers_, moving_);
}

bool ImmediateInterpreter::KeyboardRecentlyUsed(stime_t now) const {
  // For tests, values of 0 mean keyboard not used recently.
  if (keyboard_touched_ == 0.0)
    return false;
  // Sanity check. If keyboard_touched_ is more than 10 seconds away from now,
  // ignore it.
  if (fabs(now - keyboard_touched_) > 10)
    return false;

  return keyboard_touched_ + keyboard_palm_prevent_timeout_.val_ > now;
}

namespace {
struct GetGesturingFingersCompare {
  // Returns true if finger_a is strictly closer to keyboard than finger_b
  bool operator()(const FingerState* finger_a, const FingerState* finger_b) {
    return finger_a->position_y < finger_b->position_y;
  }
};
}  // namespace {}

FingerMap ImmediateInterpreter::GetGesturingFingers(
    const HardwareState& hwstate) const {
  // We support up to kMaxGesturingFingers finger gestures
  if (pointing_.size() <= kMaxGesturingFingers)
    return pointing_;

  const FingerState* fs[hwstate.finger_cnt];
  for (size_t i = 0; i < hwstate.finger_cnt; ++i)
    fs[i] = &hwstate.fingers[i];

  // Pull the kMaxSize FingerStates w/ the lowest position_y to the
  // front of fs[].
  GetGesturingFingersCompare compare;
  FingerMap ret;
  size_t sorted_cnt;
  if (hwstate.finger_cnt > kMaxGesturingFingers) {
    std::partial_sort(fs, fs + kMaxGesturingFingers, fs + hwstate.finger_cnt,
                      compare);
    sorted_cnt = kMaxGesturingFingers;
  } else {
    std::sort(fs, fs + hwstate.finger_cnt, compare);
    sorted_cnt = hwstate.finger_cnt;
  }
  for (size_t i = 0; i < sorted_cnt; i++)
    ret.insert(fs[i]->tracking_id);
  return ret;
}

void ImmediateInterpreter::UpdateCurrentGestureType(
    const HardwareState& hwstate,
    const FingerMap& gs_fingers,
    FingerMap* active_gs_fingers) {
  *active_gs_fingers = gs_fingers;

  size_t num_gesturing = gs_fingers.size();

  // Physical button or tap overrides current gesture state
  if (sent_button_down_ || tap_to_click_state_ == kTtcDrag) {
    current_gesture_type_ = kGestureTypeMove;
    return;
  }

  // current gesture state machine
  switch (current_gesture_type_) {
    case kGestureTypeContactInitiated:
    case kGestureTypeButtonsChange:
      break;

    case kGestureTypeScroll:
    case kGestureTypeSwipe:
    case kGestureTypeFourFingerSwipe:
      // Don't allow a pinch after a scroll or swipe gesture has been detected
      these_fingers_scrolled_ = true;

      // If a gesturing finger just left, do fling/lift
      if (AnyGesturingFingerLeft(*state_buffer_.Get(0),
                                 prev_gs_fingers_)) {
        current_gesture_type_ = GetFingerLiftGesture(current_gesture_type_);
        moving_.clear();
        return;
      }
      // fallthrough
    case kGestureTypeSwipeLift:
    case kGestureTypeFourFingerSwipeLift:
    case kGestureTypeFling:
    case kGestureTypeMove:
    case kGestureTypeNull:
      // When a finger leaves, we hold the gesture processing for
      // change_timeout_ time.
      if (hwstate.timestamp < finger_leave_time_ + change_timeout_.val_) {
        current_gesture_type_ = kGestureTypeNull;
        return;
      }

      // Scrolling detection for T5R2 devices
      if ((hwprops_->supports_t5r2 || hwprops_->support_semi_mt) &&
          (hwstate.touch_cnt > 2)) {
        current_gesture_type_ = kGestureTypeScroll;
        return;
      }

      // Finger gesture decision process
      if (num_gesturing == 0) {
        current_gesture_type_ = kGestureTypeNull;
      } else if (num_gesturing == 1) {
        const FingerState* finger =
            hwstate.GetFingerState(*gs_fingers.begin());
        if (PalmIsArrivingOrDeparting(*finger))
          current_gesture_type_ = kGestureTypeNull;
        else
          current_gesture_type_ = kGestureTypeMove;
      } else {
        if (changed_time_ > started_moving_time_ ||
            hwstate.timestamp - max(started_moving_time_, gs_changed_time_) <
            evaluation_timeout_.val_ ||
            current_gesture_type_ == kGestureTypeNull) {
          // Try to recognize gestures, starting from many-finger gestures
          // first. We choose this order b/c 3-finger gestures are very strict
          // in their interpretation.
          vector<short, kMaxGesturingFingers> sorted_ids;
          SortFingersByProximity(gs_fingers, hwstate, &sorted_ids);
          for (; sorted_ids.size() >= 2;
               sorted_ids.erase(sorted_ids.end() - 1)) {
            if (sorted_ids.size() == 2) {
              GestureType new_gs_type = kGestureTypeNull;
              const FingerState* fingers[] = {
                hwstate.GetFingerState(*sorted_ids.begin()),
                hwstate.GetFingerState(*(sorted_ids.begin() + 1))
              };
              if (!fingers[0] || !fingers[1]) {
                Err("Unable to find gesturing fingers!");
                return;
              }
              // See if two pointers are close together
              bool potential_two_finger_gesture =
                  TwoFingersGesturing(*fingers[0], *fingers[1], false);
              if (!potential_two_finger_gesture) {
                new_gs_type = kGestureTypeMove;
              } else {
                new_gs_type =
                    GetTwoFingerGestureType(*fingers[0], *fingers[1]);
                // Two fingers that don't end up causing scroll may be
                // ambiguous. Only move if they've been down long enough.
                if (new_gs_type == kGestureTypeMove &&
                    hwstate.timestamp -
                        min(origin_timestamps_[fingers[0]->tracking_id],
                            origin_timestamps_[fingers[1]->tracking_id]) <
                    evaluation_timeout_.val_)
                  new_gs_type = kGestureTypeNull;
              }
              if (new_gs_type != kGestureTypeMove ||
                  gs_fingers.size() == 2) {
                // We only allow this path to set a move gesture if there
                // are two fingers gesturing
                current_gesture_type_ = new_gs_type;
              }
            } else if (sorted_ids.size() == 3) {
              const FingerState* fingers[] = {
                hwstate.GetFingerState(*sorted_ids.begin()),
                hwstate.GetFingerState(*(sorted_ids.begin() + 1)),
                hwstate.GetFingerState(*(sorted_ids.begin() + 2))
              };
              if (!fingers[0] || !fingers[1] || !fingers[2]) {
                Err("Unable to find gesturing fingers!");
                return;
              }
              current_gesture_type_ = GetMultiFingerGestureType(fingers, 3);
              if (current_gesture_type_ == kGestureTypeSwipe)
                last_swipe_timestamp_ = hwstate.timestamp;
            } else if (sorted_ids.size() == 4) {
              const FingerState* fingers[] = {
                hwstate.GetFingerState(*sorted_ids.begin()),
                hwstate.GetFingerState(*(sorted_ids.begin() + 1)),
                hwstate.GetFingerState(*(sorted_ids.begin() + 2)),
                hwstate.GetFingerState(*(sorted_ids.begin() + 3))
              };
              if (!fingers[0] || !fingers[1] || !fingers[2] || !fingers[3]) {
                Err("Unable to find gesturing fingers!");
                return;
              }
              current_gesture_type_ = GetMultiFingerGestureType(fingers, 4);
              if (current_gesture_type_ == kGestureTypeFourFingerSwipe)
                current_gesture_type_ = kGestureTypeFourFingerSwipe;
            }
            if (current_gesture_type_ != kGestureTypeNull) {
              active_gs_fingers->clear();
              for (vector<short, kMaxGesturingFingers>::const_iterator it =
                   sorted_ids.begin(), e = sorted_ids.end(); it != e; ++it)
                active_gs_fingers->insert(*it);
              break;
            }
          }
        }
      }

      if ((current_gesture_type_ == kGestureTypeMove ||
           current_gesture_type_ == kGestureTypeNull) &&
          (pinch_enable_.val_ && !hwprops_->support_semi_mt) &&
          !these_fingers_scrolled_) {
        bool do_pinch = UpdatePinchState(hwstate, false, gs_fingers);
        if (do_pinch) {
          current_gesture_type_ = kGestureTypePinch;
        } else if (EarlyZoomPotential(hwstate)) {
          current_gesture_type_ = kGestureTypeNull;
        }
      }
      break;

    case kGestureTypePinch:
      if (fingers_.size() == 2 ||
          (pinch_status_ == GESTURES_ZOOM_END &&
           prev_gesture_type_ == kGestureTypePinch) ||
          (prev_gesture_type_ == kGestureTypePinch &&
           pinch_locked_ == true)) {
        return;
      } else {
        current_gesture_type_ = kGestureTypeNull;
      }
      break;

    case kGestureTypeMetrics:
      // One shouldn't reach here
      Err("Metrics gestures reached ImmediateInterpreter");
      break;
  }
  return;
}

namespace {
// Can't use tuple<float, short, short> b/c we want to make a variable
// sized array of them on the stack
struct DistSqElt {
  float dist_sq;
  short tracking_id[2];
};

struct DistSqCompare {
  // Returns true if finger_a is strictly closer to keyboard than finger_b
  bool operator()(const DistSqElt& finger_a, const DistSqElt& finger_b) {
    return finger_a.dist_sq < finger_b.dist_sq;
  }
};

}  // namespace {}

void ImmediateInterpreter::SortFingersByProximity(
    const FingerMap& finger_ids,
    const HardwareState& hwstate,
    vector<short, kMaxGesturingFingers>* out_sorted_ids) {
  if (finger_ids.size() <= 2) {
    for (short finger_id : finger_ids)
      out_sorted_ids->push_back(finger_id);
    return;
  }
  // To do the sort, we sort all inter-point distances^2, then scan through
  // that until we have enough points
  size_t dist_sq_capacity =
      (finger_ids.size() * (finger_ids.size() - 1)) / 2;
  DistSqElt dist_sq[dist_sq_capacity];
  size_t dist_sq_len = 0;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs1 = hwstate.fingers[i];
    if (!SetContainsValue(finger_ids, fs1.tracking_id))
      continue;
    for (size_t j = i + 1; j < hwstate.finger_cnt; j++) {
      const FingerState& fs2 = hwstate.fingers[j];
      if (!SetContainsValue(finger_ids, fs2.tracking_id))
        continue;
      DistSqElt elt = {
        DistSq(fs1, fs2),
        { fs1.tracking_id, fs2.tracking_id }
      };
      if (dist_sq_len >= dist_sq_capacity) {
        Err("%s: Array overrun", __func__);
        break;
      }
      dist_sq[dist_sq_len++] = elt;
    }
  }

  DistSqCompare distSqCompare;
  std::sort(dist_sq, dist_sq + dist_sq_len, distSqCompare);

  if (out_sorted_ids == NULL) {
    Err("out_sorted_ids became NULL");
    return;
  }
  for (size_t i = 0; i < dist_sq_len; i++) {
    short id1 = dist_sq[i].tracking_id[0];
    short id2 = dist_sq[i].tracking_id[1];
    bool contains1 = out_sorted_ids->find(id1) != out_sorted_ids->end();
    bool contains2 = out_sorted_ids->find(id2) != out_sorted_ids->end();
    if (contains1 == contains2 && !out_sorted_ids->empty()) {
      // Assuming we have some ids in the out vector, then either we have both
      // of these new ids, we have neither. Either way, we can't use this edge.
      continue;
    }
    if (!contains1)
      out_sorted_ids->push_back(id1);
    if (!contains2)
      out_sorted_ids->push_back(id2);
    if (out_sorted_ids->size() == finger_ids.size())
      break;  // We've got all the IDs
  }
}


bool ImmediateInterpreter::UpdatePinchState(
    const HardwareState& hwstate, bool reset, const FingerMap& gs_fingers) {

  if (reset) {
    if (pinch_locked_) {
      current_gesture_type_ = kGestureTypePinch;
      pinch_status_ = GESTURES_ZOOM_END;
    }
    // perform reset to "don't know" state
    pinch_guess_start_ = -1.0f;
    pinch_locked_ = false;
    pinch_prev_distance_sq_ = -1.0f;
    these_fingers_scrolled_ = false;
    return false;
  }

  // once locked stay locked until reset.
  if (pinch_locked_) {
    pinch_status_ = GESTURES_ZOOM_UPDATE;
    return false;
  }

  // check if we have two valid fingers
  if (gs_fingers.size() != 2) {
    return false;
  }
  const FingerState* finger1 = hwstate.GetFingerState(*(gs_fingers.begin()));
  const FingerState* finger2 =
      hwstate.GetFingerState(*(gs_fingers.begin() + 1));
  if (finger1 == NULL || finger2 == NULL) {
    Err("Finger unexpectedly NULL");
    return false;
  }

  // assign the bottom finger to finger2
  if (finger1->position_y > finger2->position_y) {
    std::swap(finger1, finger2);
  }

  // Check if the two fingers have start positions
  if (!MapContainsKey(start_positions_, finger1->tracking_id) ||
      !MapContainsKey(start_positions_, finger2->tracking_id)) {
    return false;
  }

  if (pinch_prev_distance_sq_ < 0)
    pinch_prev_distance_sq_ = TwoFingerDistanceSq(hwstate);

  // Pinch gesture detection
  //
  // The pinch gesture detection will try to make a guess about whether a pinch
  // or not-a-pinch is performed. If the guess stays valid for a specific time
  // (slow but consistent movement) or we get a certain decision (fast
  // gesturing) the decision is locked until the state is reset.
  // * A high ratio of the traveled distances between fingers indicates
  //   that a pinch is NOT performed.
  // * Strong movement of both fingers in opposite directions indicates
  //   that a pinch IS performed.

  Point delta1 = FingerTraveledVector(*finger1, false, true);
  Point delta2 = FingerTraveledVector(*finger2, false, true);

  // dot product. dot < 0 if fingers move away from each other.
  float dot = delta1.x_ * delta2.x_ + delta1.y_ * delta2.y_;
  // squared distances both finger have been traveled.
  float d1sq = delta1.x_ * delta1.x_ + delta1.y_ * delta1.y_;
  float d2sq = delta2.x_ * delta2.x_ + delta2.y_ * delta2.y_;

  // True if movement is not strong enough to be distinguished from noise.
  // This is not equivalent to a comparison of unsquared values, but seems to
  // work well in practice.
  bool movement_below_noise = (d1sq + d2sq < pinch_noise_level_sq_.val_);

  // guesses if a pinch is being performed or not.
  double guess_min_mov_sq = pinch_guess_min_movement_.val_;
  guess_min_mov_sq *= guess_min_mov_sq;
  bool guess_no = (d1sq > guess_min_mov_sq) ^ (d2sq > guess_min_mov_sq) ||
                  dot > 0;
  bool guess_yes = ((d1sq > guess_min_mov_sq || d2sq > guess_min_mov_sq) &&
                    dot < 0);
  bool pinch_certain = false;

  // true if the lower finger is in the dampened zone
  bool in_dampened_zone = origin_positions_[finger2->tracking_id].y_ >
                          hwprops_->bottom - bottom_zone_size_.val_;

  float lo_dsq;
  float hi_dsq;
  if (d1sq < d2sq) {
    lo_dsq = d1sq;
    hi_dsq = d2sq;
  } else {
    lo_dsq = d2sq;
    hi_dsq = d1sq;
  }
  bool bad_mov_ratio = lo_dsq <= hi_dsq *
                                 pinch_guess_consistent_mov_ratio_.val_ *
                                 pinch_guess_consistent_mov_ratio_.val_;

  if (!bad_mov_ratio &&
      !in_dampened_zone &&
      guess_yes &&
      !guess_no &&
      ZoomFingersAreConsistent(state_buffer_)) {
    pinch_certain = true;
  }

  // Thumb is in dampened zone: Only allow inward pinch
  if (in_dampened_zone &&
      (d2sq < pinch_thumb_min_movement_.val_ * pinch_thumb_min_movement_.val_ ||
       !InwardPinch(state_buffer_, *finger2))) {
    guess_yes = false;
    guess_no = true;
    pinch_certain = false;
  }

  // do state transitions and final decision
  if (pinch_guess_start_ < 0) {
    // "Don't Know"-state

    // Determine guess.
    if (!movement_below_noise) {
      if (guess_no && !guess_yes) {
        pinch_guess_ = false;
        pinch_guess_start_ = hwstate.timestamp;
      }
      if (guess_yes && !guess_no) {
        pinch_guess_ = true;
        pinch_guess_start_ = hwstate.timestamp;
      }
    }
  }
  if (pinch_guess_start_ >= 0) {
    // "Guessed"-state

    // suppress cursor movement when we guess a pinch gesture
    if (pinch_guess_) {
      for (size_t i = 0; i < hwstate.finger_cnt; ++i) {
        FingerState* finger_state = &hwstate.fingers[i];
        finger_state->flags |= GESTURES_FINGER_WARP_X;
        finger_state->flags |= GESTURES_FINGER_WARP_Y;
      }
    }

    // Go back to "Don't Know"-state if guess is no longer valid
    if (pinch_guess_ != guess_yes ||
        pinch_guess_ == guess_no ||
        movement_below_noise) {
      pinch_guess_start_ = -1.0f;
      return false;
    }

    // certain decisions if pinch is being performed or not
    double cert_min_mov_sq = pinch_certain_min_movement_.val_;
    cert_min_mov_sq *= cert_min_mov_sq;
    pinch_certain |= (d1sq > cert_min_mov_sq &&
                      d2sq > cert_min_mov_sq) &&
                     dot < 0;
    bool no_pinch_certain = (d1sq > cert_min_mov_sq ||
                             d2sq > cert_min_mov_sq) &&
                            dot > 0;
    pinch_guess_ |= pinch_certain;
    pinch_guess_ &= !no_pinch_certain;

    // guessed for long enough or certain decision was made: lock
    if ((hwstate.timestamp - pinch_guess_start_ >
         pinch_evaluation_timeout_.val_) ||
        pinch_certain ||
        no_pinch_certain) {
      pinch_status_ = GESTURES_ZOOM_START;
      pinch_locked_ = true;
      return pinch_guess_;
    }
  }

  return false;
}

bool ImmediateInterpreter::PalmIsArrivingOrDeparting(
    const FingerState& finger) const {
  if (((finger.flags & GESTURES_FINGER_POSSIBLE_PALM) ||
       (finger.flags & GESTURES_FINGER_PALM)) &&
      ((finger.flags & GESTURES_FINGER_TREND_INC_TOUCH_MAJOR) ||
       (finger.flags & GESTURES_FINGER_TREND_DEC_TOUCH_MAJOR)) &&
      ((finger.flags & GESTURES_FINGER_TREND_INC_PRESSURE) ||
       (finger.flags & GESTURES_FINGER_TREND_DEC_PRESSURE)))
    return true;
  return false;
}

bool ImmediateInterpreter::IsTooCloseToThumb(const FingerState& finger) const {
  const float kMin2fDistThreshSq = tapping_finger_min_separation_.val_ *
      tapping_finger_min_separation_.val_;
  for (map<short, stime_t, kMaxFingers>::const_iterator it = thumb_.begin();
       it != thumb_.end(); ++it) {
    const FingerState* thumb = state_buffer_.Get(0)->GetFingerState(it->first);
    float xdist = fabsf(finger.position_x - thumb->position_x);
    float ydist = fabsf(finger.position_y - thumb->position_y);
    if (xdist * xdist + ydist * ydist < kMin2fDistThreshSq)
      return true;
  }
  return false;
}

bool ImmediateInterpreter::TwoFingersGesturing(
    const FingerState& finger1,
    const FingerState& finger2,
    bool check_button_type) const {
  // Make sure distance between fingers isn't too great
  if (!metrics_->CloseEnoughToGesture(Vector2(finger1), Vector2(finger2)))
    return false;

  // Next, if two fingers are moving a lot, they are gesturing together.
  if (started_moving_time_ > changed_time_) {
    // Fingers are moving
    float dist1_sq = DistanceTravelledSq(finger1, false);
    float dist2_sq = DistanceTravelledSq(finger2, false);
    if (thumb_movement_factor_.val_ * thumb_movement_factor_.val_ *
        max(dist1_sq, dist2_sq) < min(dist1_sq, dist2_sq)) {
      return true;
    }
  }

  // Make sure the pressure difference isn't too great for vertically
  // aligned contacts
  float pdiff = fabsf(finger1.pressure - finger2.pressure);
  float xdist = fabsf(finger1.position_x - finger2.position_x);
  float ydist = fabsf(finger1.position_y - finger2.position_y);
  if (pdiff > two_finger_pressure_diff_thresh_.val_ && ydist > xdist &&
      ((finger1.pressure > finger2.pressure) ==
       (finger1.position_y > finger2.position_y)))
    return false;

  const float kMin2fDistThreshSq = tapping_finger_min_separation_.val_ *
      tapping_finger_min_separation_.val_;
  float dist_sq = xdist * xdist + ydist * ydist;
  // Make sure distance between fingers isn't too small
  if ((dist_sq < kMin2fDistThreshSq) &&
      !(finger1.flags & GESTURES_FINGER_MERGE))
    return false;

  // If both fingers have a tendency of moving at the same direction, they
  // are gesturing together. This check is disabled if we are using the
  // function to distinguish left/right clicks.
  if (!check_button_type) {
    unsigned and_flags = finger1.flags & finger2.flags;
    if ((and_flags & GESTURES_FINGER_TREND_INC_X) ||
        (and_flags & GESTURES_FINGER_TREND_DEC_X) ||
        (and_flags & GESTURES_FINGER_TREND_INC_Y) ||
        (and_flags & GESTURES_FINGER_TREND_DEC_Y))
      return true;
  }

  // Next, if fingers are vertically aligned and one is in the bottom zone,
  // consider that one a resting thumb (thus, do not scroll/right click)
  // if it has greater pressure. For clicking, we relax the pressure requirement
  // because we may not have enough time to determine.
  if (xdist < ydist && (FingerInDampenedZone(finger1) ||
                        FingerInDampenedZone(finger2)) &&
      (FingerInDampenedZone(finger1) == (finger1.pressure > finger2.pressure) ||
       check_button_type))
    return false;
  return true;
}

GestureType ImmediateInterpreter::GetTwoFingerGestureType(
    const FingerState& finger1,
    const FingerState& finger2) {
  if (!MapContainsKey(start_positions_, finger1.tracking_id) ||
      !MapContainsKey(start_positions_, finger2.tracking_id))
    return kGestureTypeNull;

  // If a finger is close to any thumb, we believe it to be due to thumb-splits
  // and ignore it.
  int num_close_to_thumb = 0;
  num_close_to_thumb += static_cast<int>(IsTooCloseToThumb(finger1));
  num_close_to_thumb += static_cast<int>(IsTooCloseToThumb(finger2));
  if (num_close_to_thumb == 1)
    return kGestureTypeMove;
  else if (num_close_to_thumb == 2)
    return kGestureTypeNull;

  // Compute distance traveled since fingers changed for each finger
  float dx1 = finger1.position_x - start_positions_[finger1.tracking_id].x_;
  float dy1 = finger1.position_y - start_positions_[finger1.tracking_id].y_;
  float dx2 = finger2.position_x - start_positions_[finger2.tracking_id].x_;
  float dy2 = finger2.position_y - start_positions_[finger2.tracking_id].y_;

  float large_dx = MaxMag(dx1, dx2);
  float large_dy = MaxMag(dy1, dy2);
  // These compares are okay if d{x,y}1 == d{x,y}2:
  short large_dx_id =
      (large_dx == dx1) ? finger1.tracking_id : finger2.tracking_id;
  short large_dy_id =
      (large_dy == dy1) ? finger1.tracking_id : finger2.tracking_id;
  float small_dx = MinMag(dx1, dx2);
  float small_dy = MinMag(dy1, dy2);

  short small_dx_id =
      (small_dx == dx1) ? finger1.tracking_id : finger2.tracking_id;
  short small_dy_id =
      (small_dy == dy1) ? finger1.tracking_id : finger2.tracking_id;

  bool dampened_zone_occupied = false;
  // movements of the finger in the dampened zone. If there are multiple
  // fingers in the dampened zone, dx is min(dx_1, dx_2), dy is min(dy_1, dy_2).
  float damp_dx = INFINITY;
  float damp_dy = INFINITY;
  float non_damp_dx = 0.0;
  float non_damp_dy = 0.0;
  if (FingerInDampenedZone(finger1) ||
      (finger1.flags & GESTURES_FINGER_POSSIBLE_PALM)) {
    dampened_zone_occupied = true;
    damp_dx = dx1;
    damp_dy = dy1;
    non_damp_dx = dx2;
    non_damp_dy = dy2;
  }
  if (FingerInDampenedZone(finger2) ||
      (finger2.flags & GESTURES_FINGER_POSSIBLE_PALM)) {
    dampened_zone_occupied = true;
    damp_dx = MinMag(damp_dx, dx2);
    damp_dy = MinMag(damp_dy, dy2);
    non_damp_dx = MaxMag(non_damp_dx, dx1);
    non_damp_dy = MaxMag(non_damp_dy, dy1);
  }

  // Trending in the same direction?
  const unsigned kTrendX =
      GESTURES_FINGER_TREND_INC_X | GESTURES_FINGER_TREND_DEC_X;
  const unsigned kTrendY =
      GESTURES_FINGER_TREND_INC_Y | GESTURES_FINGER_TREND_DEC_Y;
  unsigned common_trend_flags = finger1.flags & finger2.flags &
      (kTrendX | kTrendY);

  bool large_dx_moving =
      fabsf(large_dx) >= two_finger_scroll_distance_thresh_.val_ ||
      SetContainsValue(moving_, large_dx_id);
  bool large_dy_moving =
      fabsf(large_dy) >= two_finger_scroll_distance_thresh_.val_ ||
      SetContainsValue(moving_, large_dy_id);
  bool small_dx_moving =
      fabsf(small_dx) >= two_finger_scroll_distance_thresh_.val_ ||
      SetContainsValue(moving_, small_dx_id);
  bool small_dy_moving =
      fabsf(small_dy) >= two_finger_scroll_distance_thresh_.val_ ||
      SetContainsValue(moving_, small_dy_id);
  bool trend_scrolling_x = (common_trend_flags & kTrendX) &&
       large_dx_moving && small_dx_moving;
  bool trend_scrolling_y = (common_trend_flags & kTrendY) &&
       large_dy_moving && small_dy_moving;

  if (trend_scrolling_x || trend_scrolling_y) {
    if (pinch_enable_.val_ && !ScrollAngle(finger1, finger2))
         return kGestureTypeNull;
    return kGestureTypeScroll;
  }

  if (fabsf(large_dx) > fabsf(large_dy)) {
    // consider horizontal scroll
    if (fabsf(small_dx) < two_finger_scroll_distance_thresh_.val_)
      small_dx = 0.0;
    if (large_dx * small_dx <= 0.0) {
      // not same direction
      if (fabsf(large_dx) < two_finger_move_distance_thresh_.val_)
        return kGestureTypeNull;
      else
        return kGestureTypeMove;
    }
    if (fabsf(large_dx) < two_finger_scroll_distance_thresh_.val_)
      return kGestureTypeNull;
    if (dampened_zone_occupied) {
      // Require damp to move at least some amount with the other finger
      if (fabsf(damp_dx) <
          damp_scroll_min_movement_factor_.val_ * fabsf(non_damp_dx)) {
        return kGestureTypeNull;
      }
    }
    if (pinch_enable_.val_ && !ScrollAngle(finger1, finger2))
         return kGestureTypeNull;
    return kGestureTypeScroll;
  } else {
    // consider vertical scroll
    if (fabsf(small_dy) < two_finger_scroll_distance_thresh_.val_)
      small_dy = 0.0;
    if (large_dy * small_dy <= 0.0) {
      if (fabsf(large_dy) < two_finger_move_distance_thresh_.val_)
        return kGestureTypeNull;
      else
        return kGestureTypeMove;
    }
    if (dampened_zone_occupied) {
      // Require damp to move at least some amount with the other finger
      if (fabsf(damp_dy) <
          damp_scroll_min_movement_factor_.val_ * fabsf(non_damp_dy)) {
        return kGestureTypeNull;
      }
    }
    if (pinch_enable_.val_ && !ScrollAngle(finger1, finger2))
         return kGestureTypeNull;
    return kGestureTypeScroll;
  }
}

GestureType ImmediateInterpreter::GetFingerLiftGesture(
    GestureType current_gesture_type) {
  switch(current_gesture_type) {
    case kGestureTypeScroll: return kGestureTypeFling;
    case kGestureTypeSwipe: return kGestureTypeSwipeLift;
    case kGestureTypeFourFingerSwipe: return kGestureTypeFourFingerSwipeLift;
    default: return kGestureTypeNull;
  }
}

GestureType ImmediateInterpreter::GetMultiFingerGestureType(
    const FingerState* const fingers[], const int num_fingers) {
  float close_distance_thresh;
  float swipe_distance_thresh;
  float swipe_distance_ratio;
  GestureType gesture_type;
  if (num_fingers == 4) {
    close_distance_thresh = four_finger_close_distance_thresh_.val_;
    swipe_distance_thresh = four_finger_swipe_distance_thresh_.val_;
    swipe_distance_ratio = four_finger_swipe_distance_ratio_.val_;
    gesture_type = kGestureTypeFourFingerSwipe;
  } else if (num_fingers == 3) {
    close_distance_thresh = three_finger_close_distance_thresh_.val_;
    swipe_distance_thresh = three_finger_swipe_distance_thresh_.val_;
    swipe_distance_ratio = three_finger_swipe_distance_ratio_.val_;
    gesture_type = kGestureTypeSwipe;
  } else {
    return kGestureTypeNull;
  }

  const FingerState* x_fingers[num_fingers];
  const FingerState* y_fingers[num_fingers];
  for (int i = 0; i < num_fingers; i++) {
    x_fingers[i] = fingers[i];
    y_fingers[i] = fingers[i];
  }
  std::sort(x_fingers, x_fingers + num_fingers,
            [] (const FingerState* a, const FingerState* b) ->
                bool { return a->position_x < b->position_x; });
  std::sort(y_fingers, y_fingers + num_fingers,
            [] (const FingerState* a, const FingerState* b) ->
                bool { return a->position_y < b->position_y; });
  bool horizontal =
      (x_fingers[num_fingers - 1]->position_x - x_fingers[0]->position_x) >=
      (y_fingers[num_fingers -1]->position_y - y_fingers[0]->position_y);
  const FingerState* sorted_fingers[num_fingers];
  for (int i = 0; i < num_fingers; i++) {
    sorted_fingers[i] = horizontal ? x_fingers[i] : y_fingers[i];
  }
  if (DistSq(*sorted_fingers[0], *sorted_fingers[num_fingers - 1]) >
      close_distance_thresh * close_distance_thresh) {
    return kGestureTypeNull;
  }

  float dx[num_fingers];
  float dy[num_fingers];
  for (int i = 0; i < num_fingers; i++) {
    dx[i] = sorted_fingers[i]->position_x -
            start_positions_[sorted_fingers[i]->tracking_id].x_;
    dy[i] = sorted_fingers[i]->position_y -
            start_positions_[sorted_fingers[i]->tracking_id].y_;
  }
  // pick horizontal or vertical
  float *deltas = fabsf(dx[0]) > fabsf(dy[0]) ? dx : dy;
  swipe_is_vertical_ = deltas == dy;

  // All fingers must move in the same direction.
  for (int i = 1; i < num_fingers; i++) {
    if (deltas[i] * deltas[0] <= 0.0) {
      return kGestureTypeNull;
    }
  }

  // All fingers must have traveled far enough.
  float max_delta = fabsf(deltas[0]);
  float min_delta = fabsf(deltas[0]);
  for (int i = 1; i < num_fingers; i++) {
    max_delta = max(max_delta, fabsf(deltas[i]));
    min_delta = min(min_delta, fabsf(deltas[i]));
  }
  if (max_delta >= swipe_distance_thresh &&
      min_delta >= swipe_distance_ratio * max_delta)
    return gesture_type;
  return kGestureTypeNull;
}

const char* ImmediateInterpreter::TapToClickStateName(TapToClickState state) {
  switch (state) {
    case kTtcIdle: return "Idle";
    case kTtcFirstTapBegan: return "FirstTapBegan";
    case kTtcTapComplete: return "TapComplete";
    case kTtcSubsequentTapBegan: return "SubsequentTapBegan";
    case kTtcDrag: return "Drag";
    case kTtcDragRelease: return "DragRelease";
    case kTtcDragRetouch: return "DragRetouch";
    default: return "<unknown>";
  }
}

stime_t ImmediateInterpreter::TimeoutForTtcState(TapToClickState state) {
  switch (state) {
    case kTtcIdle: return tap_timeout_.val_;
    case kTtcFirstTapBegan: return tap_timeout_.val_;
    case kTtcTapComplete: return inter_tap_timeout_.val_;
    case kTtcSubsequentTapBegan: return tap_timeout_.val_;
    case kTtcDrag: return tap_timeout_.val_;
    case kTtcDragRelease: return tap_drag_timeout_.val_;
    case kTtcDragRetouch: return tap_timeout_.val_;
    default:
      Log("Unknown state!");
      return 0.0;
  }
}

void ImmediateInterpreter::SetTapToClickState(TapToClickState state,
                                              stime_t now) {
  if (tap_to_click_state_ != state) {
    tap_to_click_state_ = state;
    tap_to_click_state_entered_ = now;
  }
}

void ImmediateInterpreter::UpdateTapGesture(
    const HardwareState* hwstate,
    const FingerMap& gs_fingers,
    const bool same_fingers,
    stime_t now,
    stime_t* timeout) {
  unsigned down = 0;
  unsigned up = 0;
  UpdateTapState(hwstate, gs_fingers, same_fingers, now, &down, &up, timeout);
  if (down == 0 && up == 0) {
    return;
  }
  Log("UpdateTapGesture: Tap Generated");
  result_ = Gesture(kGestureButtonsChange,
                    state_buffer_.Get(1)->timestamp,
                    now,
                    down,
                    up);
}

void ImmediateInterpreter::UpdateTapState(
    const HardwareState* hwstate,
    const FingerMap& gs_fingers,
    const bool same_fingers,
    stime_t now,
    unsigned* buttons_down,
    unsigned* buttons_up,
    stime_t* timeout) {
  if (tap_to_click_state_ == kTtcIdle && (!tap_enable_.val_ ||
                                          tap_paused_.val_))
    return;

  FingerMap tap_gs_fingers;

  if (hwstate)
    RemoveMissingIdsFromSet(&tap_dead_fingers_, *hwstate);

  bool cancel_tapping = false;
  if (hwstate) {
    for (int i = 0; i < hwstate->finger_cnt; ++i) {
      if (hwstate->fingers[i].flags &
          (GESTURES_FINGER_NO_TAP | GESTURES_FINGER_MERGE))
        cancel_tapping = true;
    }
    for (FingerMap::const_iterator it =
             gs_fingers.begin(), e = gs_fingers.end(); it != e; ++it) {
      const FingerState* fs = hwstate->GetFingerState(*it);
      if (!fs) {
        Err("Missing finger state?!");
        continue;
      }
      tap_gs_fingers.insert(*it);
    }
  }
  set<short, kMaxTapFingers> added_fingers;

  // Fingers removed from the pad entirely
  set<short, kMaxTapFingers> removed_fingers;

  // Fingers that were gesturing, but now aren't
  set<short, kMaxFingers> dead_fingers;

  const bool phys_click_in_progress = hwstate && hwstate->buttons_down != 0 &&
    (zero_finger_click_enable_.val_ || finger_seen_shortly_after_button_down_);

  bool is_timeout = (now - tap_to_click_state_entered_ >
                     TimeoutForTtcState(tap_to_click_state_));

  if (phys_click_in_progress) {
    // Don't allow any current fingers to tap ever
    for (size_t i = 0; i < hwstate->finger_cnt; i++)
      tap_dead_fingers_.insert(hwstate->fingers[i].tracking_id);
  }

  if (hwstate && (!same_fingers || prev_tap_gs_fingers_ != tap_gs_fingers)) {
    // See if fingers were added
    for (FingerMap::const_iterator it =
             tap_gs_fingers.begin(), e = tap_gs_fingers.end(); it != e; ++it) {
      // If the finger was marked as a thumb before, it is not new.
      if (hwstate->timestamp - finger_origin_timestamp(*it) >
               thumb_click_prevention_timeout_.val_)
        continue;

      if (!SetContainsValue(prev_tap_gs_fingers_, *it)) {
        // Gesturing finger wasn't in prev state. It's new.
        const FingerState* fs = hwstate->GetFingerState(*it);
        if (FingerTooCloseToTap(*hwstate, *fs) ||
            FingerTooCloseToTap(*state_buffer_.Get(1), *fs) ||
            SetContainsValue(tap_dead_fingers_, fs->tracking_id))
          continue;
        added_fingers.insert(*it);
        Log("TTC: Added %d", *it);
      }
    }

    // See if fingers were removed or are now non-gesturing (dead)
    for (FingerMap::const_iterator it =
             prev_tap_gs_fingers_.begin(), e = prev_tap_gs_fingers_.end();
         it != e; ++it) {
      if (tap_gs_fingers.find(*it) != tap_gs_fingers.end())
        // still gesturing; neither removed nor dead
        continue;
      if (!hwstate->GetFingerState(*it)) {
        // Previously gesturing finger isn't in current state. It's gone.
        removed_fingers.insert(*it);
        Log("TTC: Removed %d", *it);
      } else {
        // Previously gesturing finger is in current state. It's dead.
        dead_fingers.insert(*it);
        Log("TTC: Dead %d", *it);
      }
    }
  }

  prev_tap_gs_fingers_ = tap_gs_fingers;

  // The state machine:

  // If you are updating the code, keep this diagram correct.
  // We have a TapRecord which stores current tap state.
  // Also, if the physical button is down or previous gesture type is scroll,
  // we go to (or stay in) Idle state.

  //     Start
  //       ↓
  //    [Idle**] <----------------------------------------------------------,
  //       ↓ added finger(s)                                                |
  //  ,>[FirstTapBegan] -<right click: send right click, timeout/movement>->|
  //  |    ↓ released all fingers                                           |
  // ,->[TapComplete*] --<timeout: send click>----------------------------->|
  // ||    | | two finger touching: send left click.                        |
  // |'----+-'                                                              |
  // |     ↓ add finger(s)                                                  |
  // |  [SubsequentTapBegan] --<timeout/move w/o delay: send click>-------->|
  // |     | | | release all fingers: send left click                       |
  // |<----+-+-'                                                            |
  // |     | `-> start non-left click: send left click; goto FirstTapBegan  |
  // |     ↓ timeout/movement with delay: send button down                  |
  // | ,->[Drag] --<detect 2 finger gesture: send button up>--------------->|
  // | |   ↓ release all fingers                                            |
  // | |  [DragRelease*]  --<timeout: send button up>---------------------->|
  // | |   ↓ add finger(s)                                                  |
  // | |  [DragRetouch]  --<remove fingers (left tap): send button up>----->|
  // | |   | | timeout/movement
  // | '---+-'
  // |     |  remove all fingers (non-left tap): send button up
  // '-----'
  //
  // * When entering TapComplete or DragRelease, we set a timer, since
  //   we will have no fingers on the pad and want to run possibly before
  //   fingers are put on the pad. Note that we use different timeouts
  //   based on which state we're in (tap_timeout_ or tap_drag_timeout_).
  // ** When entering idle, we reset the TapRecord.

  if (tap_to_click_state_ != kTtcIdle)
    Log("TTC State: %s", TapToClickStateName(tap_to_click_state_));
  if (!hwstate)
    Log("TTC: This is a timer callback");
  if (phys_click_in_progress || KeyboardRecentlyUsed(now) ||
      prev_result_.type == kGestureTypeScroll ||
      cancel_tapping) {
    Log("TTC: Forced to idle");
    SetTapToClickState(kTtcIdle, now);
    return;
  }

  switch (tap_to_click_state_) {
    case kTtcIdle:
      tap_record_.Clear();
      if (hwstate &&
          hwstate->timestamp - last_movement_timestamp_ >=
          motion_tap_prevent_timeout_.val_) {
        tap_record_.Update(
            *hwstate, *state_buffer_.Get(1), added_fingers, removed_fingers,
            dead_fingers);
        if (tap_record_.TapBegan())
          SetTapToClickState(kTtcFirstTapBegan, now);
      }
      break;
    case kTtcFirstTapBegan:
      if (is_timeout) {
        SetTapToClickState(kTtcIdle, now);
        break;
      }
      if (!hwstate) {
        Log("hwstate NULL but no timeout?!");
        break;
      }
      tap_record_.Update(
          *hwstate, *state_buffer_.Get(1), added_fingers,
          removed_fingers, dead_fingers);
      Log("TTC: Is tap? %d Is moving? %d",
          tap_record_.TapComplete(),
          tap_record_.Moving(*hwstate, tap_move_dist_.val_));
      if (tap_record_.TapComplete()) {
        if (!tap_record_.MinTapPressureMet() ||
            !tap_record_.FingersBelowMaxAge()) {
          SetTapToClickState(kTtcIdle, now);
        } else if (tap_record_.TapType() == GESTURES_BUTTON_LEFT &&
                   tap_drag_enable_.val_) {
          SetTapToClickState(kTtcTapComplete, now);
        } else {
          *buttons_down = *buttons_up = tap_record_.TapType();
          SetTapToClickState(kTtcIdle, now);
        }
      } else if (tap_record_.Moving(*hwstate, tap_move_dist_.val_)) {
        SetTapToClickState(kTtcIdle, now);
      }
      break;
    case kTtcTapComplete:
      if (!added_fingers.empty()) {

        tap_record_.Clear();
        tap_record_.Update(
            *hwstate, *state_buffer_.Get(1), added_fingers, removed_fingers,
            dead_fingers);

        // If more than one finger is touching: Send click
        // and return to FirstTapBegan state.
        if (tap_record_.TapType() != GESTURES_BUTTON_LEFT) {
          *buttons_down = *buttons_up = GESTURES_BUTTON_LEFT;
          SetTapToClickState(kTtcFirstTapBegan, now);
        } else {
          tap_drag_last_motion_time_ = now;
          tap_drag_finger_was_stationary_ = false;
          SetTapToClickState(kTtcSubsequentTapBegan, now);
        }
      } else if (is_timeout) {
        *buttons_down = *buttons_up =
            tap_record_.MinTapPressureMet() ? tap_record_.TapType() : 0;
        SetTapToClickState(kTtcIdle, now);
      }
      break;
    case kTtcSubsequentTapBegan:
      if (!is_timeout && !hwstate) {
        Log("hwstate NULL but not a timeout?!");
        break;
      }
      if (hwstate)
        tap_record_.Update(*hwstate, *state_buffer_.Get(1), added_fingers,
                           removed_fingers, dead_fingers);

      if (!tap_record_.Motionless(*hwstate, *state_buffer_.Get(1),
                                  tap_max_movement_.val_)) {
        tap_drag_last_motion_time_ = now;
      }
      if (tap_record_.TapType() == GESTURES_BUTTON_LEFT &&
          now - tap_drag_last_motion_time_ > tap_drag_stationary_time_.val_) {
        tap_drag_finger_was_stationary_ = true;
      }

      if (is_timeout || tap_record_.Moving(*hwstate, tap_move_dist_.val_)) {
        if (tap_record_.TapType() == GESTURES_BUTTON_LEFT) {
          if (is_timeout) {
            // moving with just one finger. Start dragging.
            *buttons_down = GESTURES_BUTTON_LEFT;
            SetTapToClickState(kTtcDrag, now);
          } else {
            bool drag_delay_met = (now - tap_to_click_state_entered_
                                   > tap_drag_delay_.val_);
            if (drag_delay_met && tap_drag_finger_was_stationary_) {
              *buttons_down = GESTURES_BUTTON_LEFT;
              SetTapToClickState(kTtcDrag, now);
            } else {
              *buttons_down = GESTURES_BUTTON_LEFT;
              *buttons_up = GESTURES_BUTTON_LEFT;
              SetTapToClickState(kTtcIdle, now);
            }
          }
        } else if (!tap_record_.TapComplete()) {
          // not just one finger. Send button click and go to idle.
          *buttons_down = *buttons_up = GESTURES_BUTTON_LEFT;
          SetTapToClickState(kTtcIdle, now);
        }
        break;
      }
      if (tap_record_.TapType() != GESTURES_BUTTON_LEFT) {
        // We aren't going to drag, so send left click now and handle current
        // tap afterwards.
        *buttons_down = *buttons_up = GESTURES_BUTTON_LEFT;
        SetTapToClickState(kTtcFirstTapBegan, now);
      }
      if (tap_record_.TapComplete()) {
        *buttons_down = *buttons_up = GESTURES_BUTTON_LEFT;
        SetTapToClickState(kTtcTapComplete, now);
        Log("TTC: Subsequent left tap complete");
      }
      break;
    case kTtcDrag:
      if (hwstate)
        tap_record_.Update(
            *hwstate, *state_buffer_.Get(1), added_fingers, removed_fingers,
            dead_fingers);
      if (tap_record_.TapComplete()) {
        tap_record_.Clear();
        if (drag_lock_enable_.val_) {
          SetTapToClickState(kTtcDragRelease, now);
        } else {
          *buttons_up = GESTURES_BUTTON_LEFT;
          SetTapToClickState(kTtcIdle, now);
        }
      }
      if (tap_record_.TapType() != GESTURES_BUTTON_LEFT &&
          now - tap_to_click_state_entered_ <= evaluation_timeout_.val_) {
        // We thought we were dragging, but actually we're doing a
        // non-tap-to-click multitouch gesture.
        *buttons_up = GESTURES_BUTTON_LEFT;
        SetTapToClickState(kTtcIdle, now);
      }
      break;
    case kTtcDragRelease:
      if (!added_fingers.empty()) {
        tap_record_.Update(
            *hwstate, *state_buffer_.Get(1), added_fingers, removed_fingers,
            dead_fingers);
        SetTapToClickState(kTtcDragRetouch, now);
      } else if (is_timeout) {
        *buttons_up = GESTURES_BUTTON_LEFT;
        SetTapToClickState(kTtcIdle, now);
      }
      break;
    case kTtcDragRetouch:
      if (hwstate)
        tap_record_.Update(
            *hwstate, *state_buffer_.Get(1), added_fingers, removed_fingers,
            dead_fingers);
      if (tap_record_.TapComplete()) {
        *buttons_up = GESTURES_BUTTON_LEFT;
        if (tap_record_.TapType() == GESTURES_BUTTON_LEFT)
          SetTapToClickState(kTtcIdle, now);
        else
          SetTapToClickState(kTtcTapComplete, now);
        break;
      }
      if (is_timeout) {
        SetTapToClickState(kTtcDrag, now);
        break;
      }
      if (!hwstate) {
        Log("not timeout but hwstate is NULL?!");
        break;
      }
      if (tap_record_.Moving(*hwstate, tap_move_dist_.val_))
        SetTapToClickState(kTtcDrag, now);
      break;
  }
  if (tap_to_click_state_ != kTtcIdle)
    Log("TTC: New state: %s", TapToClickStateName(tap_to_click_state_));
  // Take action based on new state:
  switch (tap_to_click_state_) {
    case kTtcTapComplete:
      *timeout = TimeoutForTtcState(tap_to_click_state_);
      break;
    case kTtcDragRelease:
      *timeout = TimeoutForTtcState(tap_to_click_state_);
      break;
    default:  // so gcc doesn't complain about missing enums
      break;
  }
}

bool ImmediateInterpreter::FingerTooCloseToTap(const HardwareState& hwstate,
                                               const FingerState& fs) {
  const float kMinAllowableSq =
      tapping_finger_min_separation_.val_ * tapping_finger_min_separation_.val_;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState* iter_fs = &hwstate.fingers[i];
    if (iter_fs->tracking_id == fs.tracking_id)
      continue;
    float dist_sq = DistSq(fs, *iter_fs);
    if (dist_sq < kMinAllowableSq)
      return true;
  }
  return false;
}

bool ImmediateInterpreter::FingerInDampenedZone(
    const FingerState& finger) const {
  // TODO(adlr): cache thresh
  float thresh = hwprops_->bottom - bottom_zone_size_.val_;
  return finger.position_y > thresh;
}

void ImmediateInterpreter::FillStartPositions(const HardwareState& hwstate) {
  RemoveMissingIdsFromMap(&origin_positions_, hwstate);

  for (short i = 0; i < hwstate.finger_cnt; i++) {
    Point point(hwstate.fingers[i].position_x,
                hwstate.fingers[i].position_y);
    start_positions_[hwstate.fingers[i].tracking_id] = point;
    if (!MapContainsKey(origin_positions_, hwstate.fingers[i].tracking_id))
      origin_positions_[hwstate.fingers[i].tracking_id] = point;
  }
}

int ImmediateInterpreter::GetButtonTypeFromPosition(
    const HardwareState& hwstate) {
  if (hwstate.finger_cnt <= 0 || hwstate.finger_cnt > 1 ||
      !button_right_click_zone_enable_.val_) {
    return GESTURES_BUTTON_LEFT;
  }

  const FingerState& fs = hwstate.fingers[0];
  if (fs.position_x > hwprops_->right - button_right_click_zone_size_.val_) {
    return GESTURES_BUTTON_RIGHT;
  }

  return GESTURES_BUTTON_LEFT;
}

int ImmediateInterpreter::EvaluateButtonType(
    const HardwareState& hwstate, stime_t button_down_time) {
  // Handle T5R2/SemiMT touchpads
  if ((hwprops_->supports_t5r2 || hwprops_->support_semi_mt) &&
      hwstate.touch_cnt > 2) {
    if (hwstate.touch_cnt - thumb_.size() == 3 &&
        three_finger_click_enable_.val_ && t5r2_three_finger_click_enable_.val_)
      return GESTURES_BUTTON_MIDDLE;
    return GESTURES_BUTTON_RIGHT;
  }

  // Just return the hardware state button, based on finger position,
  // if no further analysis is needed.
  bool finger_update = finger_button_click_.Update(hwstate, button_down_time);
  if (!finger_update && hwprops_->is_button_pad &&
      hwstate.buttons_down == GESTURES_BUTTON_LEFT) {
    return GetButtonTypeFromPosition(hwstate);
  } else if (!finger_update) {
    return hwstate.buttons_down;
  }
  Log("EvaluateButtonType: R/C/H: %d/%d/%d",
      finger_button_click_.num_recent(),
      finger_button_click_.num_cold(),
      finger_button_click_.num_hot());

  // Handle 2 finger cases:
  if (finger_button_click_.num_fingers() == 2)
    return finger_button_click_.EvaluateTwoFingerButtonType();

  // Handle cases with 3 or more fingers:
  return finger_button_click_.EvaluateThreeOrMoreFingerButtonType();
}

FingerMap ImmediateInterpreter::UpdateMovingFingers(
    const HardwareState& hwstate) {
  FingerMap newly_moving_fingers;
  if (moving_.size() == hwstate.finger_cnt)
    return newly_moving_fingers;  // All fingers already started moving
  const float kMinDistSq =
      change_move_distance_.val_ * change_move_distance_.val_;
  for (size_t i = 0; i < hwstate.finger_cnt; i++) {
    const FingerState& fs = hwstate.fingers[i];
    if (!MapContainsKey(start_positions_, fs.tracking_id)) {
      Err("Missing start position!");
      continue;
    }
    if (SetContainsValue(moving_, fs.tracking_id)) {
      // This finger already moving
      continue;
    }
    float dist_sq = DistanceTravelledSq(fs, false);
    if (dist_sq > kMinDistSq) {
      moving_.insert(fs.tracking_id);
      newly_moving_fingers.insert(fs.tracking_id);
    }
  }
  return newly_moving_fingers;
}

void ImmediateInterpreter::UpdateStartedMovingTime(
    stime_t now,
    const FingerMap& gs_fingers,
    const FingerMap& newly_moving_fingers) {
  // Update started moving time if any gesturing finger is newly moving.
  for (auto it = gs_fingers.begin(), e = gs_fingers.end(); it != e; ++it) {
    if (SetContainsValue(newly_moving_fingers, *it)) {
      started_moving_time_ = now;
      // Extend the thumb evaluation period for any finger that is still under
      // evaluation as there is a new moving finger.
      for (map<short, stime_t, kMaxFingers>::iterator it = thumb_.begin();
           it != thumb_.end(); ++it)
        if ((*it).second < thumb_eval_timeout_.val_ && (*it).second > 0.0)
          (*it).second = thumb_eval_timeout_.val_;
      return;
    }
  }
}

void ImmediateInterpreter::UpdateButtons(const HardwareState& hwstate,
                                         stime_t* timeout) {
  // TODO(miletus): To distinguish between left/right buttons down
  bool prev_button_down = state_buffer_.Get(1)->buttons_down;
  bool button_down = hwstate.buttons_down;
  if (!prev_button_down && !button_down)
    return;
  bool phys_down_edge = button_down && !prev_button_down;
  bool phys_up_edge = !button_down && prev_button_down;
  if (phys_down_edge) {
    finger_seen_shortly_after_button_down_ = false;
    sent_button_down_ = false;
    button_down_timeout_ = hwstate.timestamp + button_evaluation_timeout_.val_;
  }

  // If we haven't seen a finger on the pad shortly after the click, do nothing
  if (!finger_seen_shortly_after_button_down_ &&
      hwstate.timestamp <= button_down_timeout_)
    finger_seen_shortly_after_button_down_ = (hwstate.finger_cnt > 0);
  if (!finger_seen_shortly_after_button_down_ &&
      !zero_finger_click_enable_.val_)
    return;

  if (!sent_button_down_) {
    stime_t button_down_time = button_down_timeout_ -
                               button_evaluation_timeout_.val_;
    button_type_ = EvaluateButtonType(hwstate, button_down_time);

    if (!hwstate.SameFingersAs(*state_buffer_.Get(0))) {
      // Fingers have changed since last state, reset timeout
      button_down_timeout_ = hwstate.timestamp + button_finger_timeout_.val_;
    }

    // button_up before button_evaluation_timeout_ expired.
    // Send up & down for button that was previously down, but not yet sent.
    if (button_type_ == GESTURES_BUTTON_NONE)
      button_type_ = prev_button_down;
    // Send button down if timeout has been reached or button up happened
    if (button_down_timeout_ <= hwstate.timestamp ||
        phys_up_edge) {
      // Send button down
      if (result_.type == kGestureTypeButtonsChange)
        Err("Gesture type already button?!");
      result_ = Gesture(kGestureButtonsChange,
                        state_buffer_.Get(1)->timestamp,
                        hwstate.timestamp,
                        button_type_,
                        0);
      sent_button_down_ = true;
    } else if (timeout) {
      *timeout = button_down_timeout_ - hwstate.timestamp;
    }
  }
  if (phys_up_edge) {
    // Send button up
    if (result_.type != kGestureTypeButtonsChange)
      result_ = Gesture(kGestureButtonsChange,
                        state_buffer_.Get(1)->timestamp,
                        hwstate.timestamp,
                        0,
                        button_type_);
    else
      result_.details.buttons.up = button_type_;
    // Reset button state
    button_type_ = GESTURES_BUTTON_NONE;
    button_down_timeout_ = 0;
    sent_button_down_ = false;
    // When a buttons_up event is generated, we need to reset the
    // finger_leave_time_ in order to defer any gesture generation
    // right after it.
    finger_leave_time_ = hwstate.timestamp;
  }
}

void ImmediateInterpreter::UpdateButtonsTimeout(stime_t now) {
  if (sent_button_down_) {
    Err("How is sent_button_down_ set?");
    return;
  }
  if (button_type_ == GESTURES_BUTTON_NONE)
    return;
  sent_button_down_ = true;
  result_ = Gesture(kGestureButtonsChange,
                    state_buffer_.Get(1)->timestamp,
                    now,
                    button_type_,
                    0);
}

void ImmediateInterpreter::FillResultGesture(
    const HardwareState& hwstate,
    const FingerMap& fingers) {
  bool zero_move = false;
  switch (current_gesture_type_) {
    case kGestureTypeMove: {
      if (fingers.empty())
        return;
      // Use the finger which has moved the most to compute motion.
      // First, need to find out which finger that is.
      const FingerState* current = NULL;
      if (moving_finger_id_ >= 0)
        current = hwstate.GetFingerState(moving_finger_id_);

      const HardwareState* prev_hs = state_buffer_.Get(1);
      if (prev_hs && !current) {
        float curr_dist_sq = -1;
        for (FingerMap::const_iterator it =
                 fingers.begin(), e = fingers.end(); it != e; ++it) {
          const FingerState* fs = hwstate.GetFingerState(*it);
          const FingerState* prev_fs = prev_hs->GetFingerState(fs->tracking_id);
          if (!prev_fs)
            break;
          float dist_sq = DistSq(*fs, *prev_fs);
          if (dist_sq > curr_dist_sq) {
            current = fs;
            curr_dist_sq = dist_sq;
          }
        }
      }
      if (!current)
        return;

      // Find corresponding finger id in previous state
      const FingerState* prev =
          state_buffer_.Get(1)->GetFingerState(current->tracking_id);
      const FingerState* prev2 = !state_buffer_.Get(2) ? NULL :
          state_buffer_.Get(2)->GetFingerState(current->tracking_id);
      if (!prev || !current)
        return;
      if (current->flags & GESTURES_FINGER_MERGE)
        return;
      stime_t dt = hwstate.timestamp - state_buffer_.Get(1)->timestamp;
      bool suppress_finger_movement =
          scroll_manager_.SuppressStationaryFingerMovement(
              *current, *prev, dt) ||
          scroll_manager_.StationaryFingerPressureChangingSignificantly(
              state_buffer_, *current);
      if (quick_acceleration_factor_.val_ && prev2) {
        stime_t dt2 =
            state_buffer_.Get(1)->timestamp - state_buffer_.Get(2)->timestamp;
        float dist_sq = DistSq(*current, *prev);
        float dist_sq2 = DistSq(*prev, *prev2);
        if (dist_sq2 * dt &&  // have prev dist and current time
            dist_sq2 * dt * dt *
            quick_acceleration_factor_.val_ * quick_acceleration_factor_.val_ <
            dist_sq * dt2 * dt2) {
          return;
        }
      }
      if (suppress_finger_movement) {
        scroll_manager_.prev_result_suppress_finger_movement_ = true;
        return;
      }
      scroll_manager_.prev_result_suppress_finger_movement_ = false;
      float dx = current->position_x - prev->position_x;
      if (current->flags & GESTURES_FINGER_WARP_X_MOVE)
        dx = 0.0;
      float dy = current->position_y - prev->position_y;
      if (current->flags & GESTURES_FINGER_WARP_Y_MOVE)
        dy = 0.0;
      float dsq = dx * dx + dy * dy;
      float dx_total = current->position_x -
                       start_positions_[current->tracking_id].x_;
      float dy_total = current->position_y -
                       start_positions_[current->tracking_id].y_;
      float dsq_total = dx_total * dx_total + dy_total * dy_total;

      float dsq_thresh = (move_lock_speed_.val_ * move_lock_speed_.val_) *
                         (dt * dt);
      if (dsq > dsq_thresh) {
        // lock onto this finger
        moving_finger_id_ = current->tracking_id;
      }

      float dsq_total_thresh =
          move_report_distance_.val_ * move_report_distance_.val_;
      if (dsq_total >= dsq_total_thresh) {
        zero_move = dsq == 0.0;
        result_ = Gesture(kGestureMove,
                          state_buffer_.Get(1)->timestamp,
                          hwstate.timestamp,
                          dx,
                          dy);
      }
      break;
    }
    case kGestureTypeScroll: {
      if (!scroll_manager_.ComputeScroll(state_buffer_,
                                         prev_active_gs_fingers_,
                                         fingers,
                                         prev_gesture_type_,
                                         prev_result_,
                                         &result_,
                                         &scroll_buffer_))
        return;
      break;
    }
    case kGestureTypeFling: {
      scroll_manager_.ComputeFling(state_buffer_, scroll_buffer_, &result_);
      break;
    }
    case kGestureTypeSwipe:
    case kGestureTypeFourFingerSwipe: {
      if (!three_finger_swipe_enable_.val_)
        break;
      float sum_delta[] = { 0.0, 0.0 };
      bool valid[] = { true, true };
      float finger_cnt[] = { 0.0, 0.0 };
      float FingerState::*fields[] = { &FingerState::position_x,
                                       &FingerState::position_y };
      for (FingerMap::const_iterator it =
               fingers.begin(), e = fingers.end(); it != e; ++it) {
        if (!state_buffer_.Get(1)->GetFingerState(*it)) {
          Err("missing prev state?");
          continue;
        }
        // We have this loop in case we want to compute diagonal swipes at
        // some point, even if currently we go with just one axis.
        for (size_t i = 0; i < arraysize(fields); i++) {
          bool correct_axis = (i == 1) == swipe_is_vertical_;
          if (!valid[i] || !correct_axis)
            continue;
          float FingerState::*field = fields[i];
          float delta = hwstate.GetFingerState(*it)->*field -
              state_buffer_.Get(1)->GetFingerState(*it)->*field;
          // The multiply is to see if they have the same sign:
          if (sum_delta[i] == 0.0 || sum_delta[i] * delta > 0) {
            sum_delta[i] += delta;
            finger_cnt[i] += 1.0;
          } else {
            sum_delta[i] = 0.0;
            valid[i] = false;
          }
        }
      }
      if (current_gesture_type_ == kGestureTypeSwipe) {
        result_ = Gesture(
            kGestureSwipe, state_buffer_.Get(1)->timestamp,
            hwstate.timestamp,
            (!swipe_is_vertical_ && finger_cnt[0]) ?
            sum_delta[0] / finger_cnt[0] : 0.0,
            (swipe_is_vertical_ && finger_cnt[1]) ?
            sum_delta[1] / finger_cnt[1] : 0.0);
      } else if (current_gesture_type_ == kGestureTypeFourFingerSwipe) {
        result_ = Gesture(
            kGestureFourFingerSwipe, state_buffer_.Get(1)->timestamp,
            hwstate.timestamp,
            (!swipe_is_vertical_ && finger_cnt[0]) ?
            sum_delta[0] / finger_cnt[0] : 0.0,
            (swipe_is_vertical_ && finger_cnt[1]) ?
            sum_delta[1] / finger_cnt[1] : 0.0);
      }
      break;
    }
    case kGestureTypeSwipeLift: {
      result_ = Gesture(kGestureSwipeLift,
                        state_buffer_.Get(1)->timestamp,
                        hwstate.timestamp);
      break;
    }

    case kGestureTypeFourFingerSwipeLift: {
      result_ = Gesture(kGestureFourFingerSwipeLift,
                        state_buffer_.Get(1)->timestamp,
                        hwstate.timestamp);
      break;
    }
    case kGestureTypePinch: {
      if (pinch_status_ == GESTURES_ZOOM_START ||
          (pinch_status_ == GESTURES_ZOOM_END &&
           prev_gesture_type_ == kGestureTypePinch)) {
        result_ = Gesture(kGesturePinch, changed_time_, hwstate.timestamp,
                          1.0, pinch_status_);
        pinch_prev_time_ = hwstate.timestamp;
        if (pinch_status_ == GESTURES_ZOOM_END) {
          current_gesture_type_ = kGestureTypeNull;
          pinch_prev_direction_ = 0;
        }
      } else {
        float current_dist_sq = TwoSpecificFingerDistanceSq(hwstate, fingers);
        if (current_dist_sq < 0) {
          current_dist_sq = pinch_prev_distance_sq_;
        }

        // Check if pinch scale has changed enough since last update to send a
        // new update.  To prevent stationary jitter, we always require the
        // scale to change by at least a small amount.  We require more change
        // if the pinch has been stationary or changed direction recently.
        float jitter_threshold = pinch_res_.val_;
        if (hwstate.timestamp - pinch_prev_time_ > pinch_stationary_time_.val_)
          jitter_threshold = pinch_stationary_res_.val_;
        if ((current_dist_sq - pinch_prev_distance_sq_) *
            pinch_prev_direction_ < 0)
          jitter_threshold = jitter_threshold > pinch_hysteresis_res_.val_ ?
                             jitter_threshold :
                             pinch_hysteresis_res_.val_;
        bool above_jitter_threshold =
            (pinch_prev_distance_sq_ > jitter_threshold * current_dist_sq ||
             current_dist_sq > jitter_threshold * pinch_prev_distance_sq_);

        if (above_jitter_threshold) {
          result_ = Gesture(kGesturePinch, changed_time_, hwstate.timestamp,
                            sqrt(current_dist_sq / pinch_prev_distance_sq_),
                            GESTURES_ZOOM_UPDATE);
          pinch_prev_direction_ =
              current_dist_sq > pinch_prev_distance_sq_ ? 1 : -1;
          pinch_prev_distance_sq_ = current_dist_sq;
          pinch_prev_time_ = hwstate.timestamp;
        }
      }
      if (pinch_status_ == GESTURES_ZOOM_START) {
        pinch_status_ = GESTURES_ZOOM_UPDATE;
        // If there is a slow pinch, it may take a little while to detect it,
        // allowing the fingers to travel a significant distance, and causing an
        // inappropriately large scale in a single frame, followed by slow
        // scaling. Here we reduce the initial scale factor depending on how
        // quickly we detected the pinch.
        float current_dist_sq = TwoSpecificFingerDistanceSq(hwstate, fingers);
        float pinch_slowness_ratio = (hwstate.timestamp - changed_time_) *
                                     pinch_initial_scale_time_inv_.val_;
        pinch_slowness_ratio = fmin(1.0, pinch_slowness_ratio);
        pinch_prev_distance_sq_ =
            (pinch_slowness_ratio * current_dist_sq) +
            ((1 - pinch_slowness_ratio) * pinch_prev_distance_sq_);
      }
      break;
    }
    default:
      result_.type = kGestureTypeNull;
  }
  scroll_manager_.UpdateScrollEventBuffer(current_gesture_type_,
                                          &scroll_buffer_);
  if ((result_.type == kGestureTypeMove && !zero_move) ||
      result_.type == kGestureTypeScroll)
    last_movement_timestamp_ = hwstate.timestamp;
}

void ImmediateInterpreter::IntWasWritten(IntProperty* prop) {
  if (prop == &keyboard_touched_timeval_low_) {
    struct timeval tv = {
      keyboard_touched_timeval_high_.val_,
      keyboard_touched_timeval_low_.val_
    };
    keyboard_touched_ = StimeFromTimeval(&tv);
  }
}

void ImmediateInterpreter::Initialize(const HardwareProperties* hwprops,
                                      Metrics* metrics,
                                      MetricsProperties* mprops,
                                      GestureConsumer* consumer) {
  Interpreter::Initialize(hwprops, metrics, mprops, consumer);
  state_buffer_.Reset(hwprops_->max_finger_cnt);
}

bool AnyGesturingFingerLeft(const HardwareState& state,
                            const FingerMap& prev_gs_fingers) {
  for (FingerMap::const_iterator it = prev_gs_fingers.begin(),
                                 e = prev_gs_fingers.end(); it != e; ++it) {
    if (!state.GetFingerState(*it)) {
      return true;
    }
  }
  return false;
}

}  // namespace gestures
