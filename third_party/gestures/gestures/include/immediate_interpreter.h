// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/macros.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/map.h"
#include "gestures/include/set.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_IMMEDIATE_INTERPRETER_H_
#define GESTURES_IMMEDIATE_INTERPRETER_H_

namespace gestures {

typedef set<short, kMaxGesturingFingers> FingerMap;

// This interpreter keeps some memory of the past and, for each incoming
// frame of hardware state, immediately determines the gestures to the best
// of its abilities.

class ImmediateInterpreter;
class MultitouchMouseInterpreter;

class TapRecord {
 public:
  explicit TapRecord(const ImmediateInterpreter* immediate_interpreter)
      : immediate_interpreter_(immediate_interpreter),
        t5r2_(false),
        t5r2_touched_size_(0),
        t5r2_released_size_(0),
        fingers_below_max_age_(true) {}
  void Update(const HardwareState& hwstate,
              const HardwareState& prev_hwstate,
              const set<short, kMaxTapFingers>& added,
              const set<short, kMaxTapFingers>& removed,
              const set<short, kMaxFingers>& dead);
  void Clear();

  // if any gesturing fingers are moving
  bool Moving(const HardwareState& hwstate, const float dist_max) const;
  bool Motionless(const HardwareState& hwstate,
                  const HardwareState& prev_hwstate,
                  const float max_speed) const;

  bool TapBegan() const;  // if a tap has begun
  bool TapComplete() const;  // is a completed tap
  // return GESTURES_BUTTON_* value or 0, if tap was too light
  int TapType() const;
  // If any contact has met the minimum pressure threshold
  bool MinTapPressureMet() const;
  bool FingersBelowMaxAge() const;
 private:
  void NoteTouch(short the_id, const FingerState& fs);  // Adds to touched_
  void NoteRelease(short the_id);  // Adds to released_
  void Remove(short the_id);  // Removes from touched_ and released_

  float CotapMinPressure() const;

  map<short, FingerState, kMaxTapFingers> touched_;
  set<short, kMaxTapFingers> released_;
  // At least one finger must meet the minimum pressure requirement during a
  // tap. This set contains the fingers that have.
  set<short, kMaxTapFingers> min_tap_pressure_met_;
  // All fingers must meet the cotap pressure, which is half of the min tap
  // pressure.
  set<short, kMaxTapFingers> min_cotap_pressure_met_;
  // Used to fetch properties
  const ImmediateInterpreter* immediate_interpreter_;
  // T5R2: For these pads, we try to track individual IDs, but if we get an
  // input event with insufficient data, we switch into T5R2 mode, where we
  // just track the number of contacts. We still maintain the non-T5R2 records
  // which are useful for tracking if contacts move a lot.
  // The following are for T5R2 mode:
  bool t5r2_;  // if set, use T5R2 hacks
  unsigned short t5r2_touched_size_;  // number of contacts that have arrived
  unsigned short t5r2_released_size_;  // number of contacts that have left
  // Whether all the fingers have age less than "Tap Maximum Finger Age".
  bool fingers_below_max_age_;
};

struct ScrollEvent {
  float dx, dy, dt;
  static ScrollEvent Add(const ScrollEvent& evt_a, const ScrollEvent& evt_b);
};
class ScrollEventBuffer {
 public:
  explicit ScrollEventBuffer(size_t size)
      : buf_(new ScrollEvent[size]), max_size_(size), size_(0), head_(0) {}
  void Insert(float dx, float dy, float dt);
  void Clear();
  size_t Size() const { return size_; }
  // 0 is newest, 1 is next newest, ..., size_ - 1 is oldest.
  const ScrollEvent& Get(size_t offset) const;
  // For efficiency, returns dist_sq and time of the last num_events events in
  // the buffer, from which speed can be computed.
  void GetSpeedSq(size_t num_events, float* dist_sq, float* dt) const;

 private:
  std::unique_ptr<ScrollEvent[]> buf_;
  size_t max_size_;
  size_t size_;
  size_t head_;
  DISALLOW_COPY_AND_ASSIGN(ScrollEventBuffer);
};

// Circular buffer for storing a rolling backlog of events for analysis
// as well as accessor functions for using the buffer's contents.
class HardwareStateBuffer {
 public:
  explicit HardwareStateBuffer(size_t size);
  ~HardwareStateBuffer();

  size_t Size() const { return size_; }

  void Reset(size_t max_finger_cnt);

  // Does a deep copy of state into states_
  void PushState(const HardwareState& state);
  // Pops most recently pushed state
  void PopState();

  const HardwareState* Get(size_t idx) const {
    return &states_[(idx + newest_index_) % size_];
  }

  HardwareState* Get(size_t idx) {
    return const_cast<HardwareState*>(
        const_cast<const HardwareStateBuffer*>(this)->Get(idx));
  }

 private:
  std::unique_ptr<HardwareState[]> states_;
  size_t newest_index_;
  size_t size_;
  size_t max_finger_cnt_;
  DISALLOW_COPY_AND_ASSIGN(HardwareStateBuffer);
};

// Helper class for compute scroll and fling.
class ScrollManager {
  FRIEND_TEST(ImmediateInterpreterTest, FlingDepthTest);
  FRIEND_TEST(MultitouchMouseInterpreterTest, SimpleTest);

 public:
  explicit ScrollManager(PropRegistry* prop_reg);
  ~ScrollManager() {}

  // Returns true if a finger's movement should be suppressed based on
  // max_stationary_move_* properties below.
  bool SuppressStationaryFingerMovement(const FingerState& fs,
                                        const FingerState& prev,
                                        stime_t dt);

  // Looking at this finger and the previous ones within a small window
  // and returns true iff this finger is stationary and the pressure is
  // changing so quickly that we expect it's arriving on the pad or
  // departing.
  bool StationaryFingerPressureChangingSignificantly(
      const HardwareStateBuffer& state_buffer,
      const FingerState& current) const;

  // Compute a scroll result.  Return false when something goes wrong.
  bool ComputeScroll(const HardwareStateBuffer& state_buffer,
                     const FingerMap& prev_gs_fingers,
                     const FingerMap& gs_fingers,
                     GestureType prev_gesture_type,
                     const Gesture& prev_result,
                     Gesture* result,
                     ScrollEventBuffer* scroll_buffer);

  // Compute a ScrollEvent that can be turned directly into a fling.
  void ComputeFling(const HardwareStateBuffer& state_buffer,
                    const ScrollEventBuffer& scroll_buffer,
                    Gesture* result) const;

  // Update ScrollEventBuffer when the current gesture type is not scroll.
  void UpdateScrollEventBuffer(GestureType gesture_type,
                               ScrollEventBuffer* scroll_buffer) const;

  void ResetSameFingerState() {
    stationary_move_distance_.clear();
  }

  // Set to true when a scroll or move is blocked b/c of high pressure
  // change or small movement. Cleared when a normal scroll or move
  // goes through.
  bool prev_result_suppress_finger_movement_;

 private:
  // Set to true when generating a non-zero scroll gesture. Reset to false
  // when a fling is generated.
  bool did_generate_scroll_;

  // Returns the number of most recent event events in the scroll_buffer_ that
  // should be considered for fling. If it returns 0, there should be no fling.
  size_t ScrollEventsForFlingCount(const ScrollEventBuffer& scroll_buffer)
    const;

  // Returns a ScrollEvent that contains velocity estimates for x and y based
  // on an N-point linear regression.
  void RegressScrollVelocity(const ScrollEventBuffer& scroll_buffer,
                             int count, ScrollEvent* out) const;

  // In addition to checking for large pressure changes when moving
  // slow, we can suppress all motion under a certain speed, unless
  // the total distance exceeds a threshold.
  DoubleProperty max_stationary_move_speed_;
  DoubleProperty max_stationary_move_speed_hysteresis_;
  DoubleProperty max_stationary_move_suppress_distance_;
  map<short, float, kMaxFingers> stationary_move_distance_;

  // A finger must change in pressure by less than this per second to trigger
  // motion.
  DoubleProperty max_pressure_change_;
  // If a contact crosses max_pressure_change_, motion continues to be blocked
  // until the pressure change per second goes below
  // max_pressure_change_hysteresis_.
  DoubleProperty max_pressure_change_hysteresis_;
  // When a high pressure change occurs during a scroll, we'll repeat
  // the last scroll if it's above this length.
  DoubleProperty min_scroll_dead_reckoning_;
  // Try to look over a period up to this length of time when looking for large
  // pressure change.
  DoubleProperty max_pressure_change_duration_;
  // A fast-swiping finger may generate rapidly changing pressure and we should
  // not report a high pressure change in this case.  This is the maximum
  // speed [mm/s] for which we may consider a finger stationary.
  DoubleProperty max_stationary_speed_;

  // y| V  /
  //  |   /  D   _-
  //  |  /    _-'
  //  | /  _-'
  //  |/_-'   H
  //  |'____________x
  // The above quadrant of a cartesian plane shows the angles where we snap
  // scrolling to vertical or horizontal. Very Vertical or Horizontal scrolls
  // are snapped, while Diagonal scrolls are not. The two properties below
  // are the slopes for the two lines.
  DoubleProperty vertical_scroll_snap_slope_;
  DoubleProperty horizontal_scroll_snap_slope_;

  // Depth of recent scroll event buffer used to compute Fling velocity.
  // For most systems this will be 3.  However, for systems that use 2x
  // interpolation, this should be 6, to ensure that the scroll events for 3
  // actual hardware states are used.
  IntProperty fling_buffer_depth_;
  // Some platforms report fingers as perfectly stationary for a few frames
  // before they report lift off. We don't include these non-movement
  // frames in the scroll buffer, because that would suppress fling.
  // Platforms with this property should set
  // fling_buffer_suppress_zero_length_scrolls_ to non-zero.
  BoolProperty fling_buffer_suppress_zero_length_scrolls_;
  // When computing a fling, if the fling buffer has an average speed under
  // this threshold, we do not perform a fling. Units are mm/sec.
  DoubleProperty fling_buffer_min_avg_speed_;
};

// Helper class for computing the button type of multi-finger clicks.
class FingerButtonClick {
 public:
  // Describes the three classes of fingers we deal with while determining
  // the type of physical button clicks.
  enum FingerClickStatus {
    // A 'recent' finger has recently touched down on the touchpad.
    STATUS_RECENT,
    // A 'cold' finger has already been on the touchpad for a while,
    // but has not been moved.
    STATUS_COLD,
    // A 'hot' finger has been moved since it touched down.
    STATUS_HOT
  };

  explicit FingerButtonClick(const ImmediateInterpreter* interpreter);
  ~FingerButtonClick() {};

  // Processes the HardwareState finger data. Categorizes fingers into one of
  // the FingerClickStatus and sort them according to their original timestamps.
  // Returns true if further analysis is needed. Returns false in trivial cases
  // where one is safe to use the HardwareState button data directly.
  bool Update(const HardwareState& hwstate, stime_t button_down_time);

  // Returns which button type corresponds to which touch count (e.g. 2f = right
  // click, 3f = middle click).
  int GetButtonTypeForTouchCount(int touch_count) const;

  // All these following button type evaluation functions are guaranteed to
  // return a button but the caller must ensure that the requirements are met.
  //
  // Evaluates the button type for the 2f case. Needs at least 2 fingers.
  int EvaluateTwoFingerButtonType();

  // Evaluates the button type for >=3f cases. Needs at least 3 fingers.
  int EvaluateThreeOrMoreFingerButtonType();

  // Evaluates the button type using finger locations.
  int EvaluateButtonTypeUsingFigureLocation();

  int num_fingers() const { return num_fingers_; }
  int num_recent() const { return num_recent_; }
  int num_cold() const { return num_cold_; }
  int num_hot() const { return num_hot_; }

 private:
  // Used to fetch properties and other finger status.
  const ImmediateInterpreter* interpreter_;

  // Fingers that we are considering for determining the button type.
  FingerState const * fingers_[4];

  // FingerClickStatus of each finger.
  FingerClickStatus fingers_status_[4];

  // Number of fingers we are considering.
  int num_fingers_;

  // Number of fingers of each kind.
  int num_recent_;
  int num_cold_;
  int num_hot_;
};

class ImmediateInterpreter : public Interpreter, public PropertyDelegate {
  FRIEND_TEST(ImmediateInterpreterTest, AmbiguousPalmCoScrollTest);
  FRIEND_TEST(ImmediateInterpreterTest, AvoidAccidentalPinchTest);
  FRIEND_TEST(ImmediateInterpreterTest, ChangeTimeoutTest);
  FRIEND_TEST(ImmediateInterpreterTest, ClickTest);
  FRIEND_TEST(ImmediateInterpreterTest, FlingDepthTest);
  FRIEND_TEST(ImmediateInterpreterTest, GetGesturingFingersTest);
  FRIEND_TEST(ImmediateInterpreterTest, PalmAtEdgeTest);
  FRIEND_TEST(ImmediateInterpreterTest, PalmReevaluateTest);
  FRIEND_TEST(ImmediateInterpreterTest, PalmTest);
  FRIEND_TEST(ImmediateInterpreterTest, DISABLED_PinchTests);
  FRIEND_TEST(ImmediateInterpreterTest, ScrollResetTapTest);
  FRIEND_TEST(ImmediateInterpreterTest, ScrollThenFalseTapTest);
  FRIEND_TEST(ImmediateInterpreterTest, SemiMtActiveAreaTest);
  FRIEND_TEST(ImmediateInterpreterTest, SemiMtNoPinchTest);
  FRIEND_TEST(ImmediateInterpreterTest, StationaryPalmTest);
  FRIEND_TEST(ImmediateInterpreterTest, SwipeTest);
  FRIEND_TEST(ImmediateInterpreterTest, TapRecordTest);
  FRIEND_TEST(ImmediateInterpreterTest, TapToClickEnableTest);
  FRIEND_TEST(ImmediateInterpreterTest, TapToClickKeyboardTest);
  FRIEND_TEST(ImmediateInterpreterTest, TapToClickLowPressureBeginOrEndTest);
  FRIEND_TEST(ImmediateInterpreterTest, TapToClickStateMachineTest);
  FRIEND_TEST(ImmediateInterpreterTest, ThumbRetainReevaluateTest);
  FRIEND_TEST(ImmediateInterpreterTest, ThumbRetainTest);
  FRIEND_TEST(ImmediateInterpreterTest, WarpedFingersTappingTest);
  friend class TapRecord;
  friend class FingerButtonClick;

 public:
  struct Point {
    Point() : x_(0.0), y_(0.0) {}
    Point(float x, float y) : x_(x), y_(y) {}
    bool operator==(const Point& that) const {
      return x_ == that.x_ && y_ == that.y_;
    }
    bool operator!=(const Point& that) const { return !((*this) == that); }
    float x_, y_;
  };
  enum TapToClickState {
    kTtcIdle,
    kTtcFirstTapBegan,
    kTtcTapComplete,
    kTtcSubsequentTapBegan,
    kTtcDrag,
    kTtcDragRelease,
    kTtcDragRetouch
  };

  ImmediateInterpreter(PropRegistry* prop_reg, Tracer* tracer);
  virtual ~ImmediateInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

  virtual void HandleTimerImpl(stime_t now, stime_t* timeout);

  virtual void Initialize(const HardwareProperties* hwprops,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);

 public:
  TapToClickState tap_to_click_state() const { return tap_to_click_state_; }

  float tap_min_pressure() const { return tap_min_pressure_.val_; }

  stime_t tap_max_finger_age() const { return tap_max_finger_age_.val_; }

  stime_t finger_origin_timestamp(short finger_id) const {
    return origin_timestamps_[finger_id];
  }

 private:
  // Reset the member variables corresponding to same-finger state and
  // updates changed_time_ to |now|.
  void ResetSameFingersState(const HardwareState& hwstate);

  // Sets pointing_.
  void UpdatePointingFingers(const HardwareState& hwstate);

  // Gets the hardware button type (RIGHT, LEFT) based on the
  // first finger's position.
  int GetButtonTypeFromPosition(const HardwareState& hwstate);

  // Returns the square of the distance that this contact has travelled since
  // fingers changed (origin=false) or since they touched down (origin=true).
  // If permit_warp is true, we ignore the GESTURES_FINGER_WARP_X/Y flags
  // unless the more strict GESTURES_FINGER_WARP_TELEPORTATION flag is set.
  float DistanceTravelledSq(const FingerState& fs,
                            bool origin,
                            bool permit_warp = false) const;

  // Returns the vector that this finger has travelled since
  // fingers changed (origin=false) or since they touched down (origin=true).
  // If permit_warp is true, we ignore the GESTURES_FINGER_WARP_X/Y flags
  // unless the more strict GESTURES_FINGER_WARP_TELEPORTATION flag is set.
  Point FingerTraveledVector(const FingerState& fs,
                             bool origin,
                             bool permit_warp = false) const;

  // Returns true if there is a potential for pinch zoom but still it's too
  // early to decide. In this case, there shouldn't be any move or scroll
  // event.
  bool EarlyZoomPotential(const HardwareState& hwstate) const;

  // Returns true if there are two fingers moving in opposite directions.
  // Moreover, this function makes sure that fingers moving direction hasn't
  // changed recently.
  bool ZoomFingersAreConsistent(const HardwareStateBuffer& state_buffer) const;

  // Returns true if the given finger is moving sufficiently upwards to be
  // considered the bottom finger of an inward pinch.
  bool InwardPinch(const HardwareStateBuffer& state_buffer,
                   const FingerState& fs) const;

  // Returns Cos(A) where A is the angle between the move vector of two fingers
  float FingersAngle(const FingerState* before1, const FingerState* before2,
                     const FingerState* curr1, const FingerState* curr2) const;

  // Returns true if fingers are not moving in opposite directions.
  bool ScrollAngle(const FingerState& finger1, const FingerState& finger2);

  // Returns the square of distance between two fingers.
  // Returns -1 if not exactly two fingers are present.
  float TwoFingerDistanceSq(const HardwareState& hwstate) const;

  // Returns the square of distance between two given fingers.
  // Returns -1 if fingers don't present in the hwstate.
  float TwoSpecificFingerDistanceSq(const HardwareState& hwstate,
                                    const FingerMap& fingers) const;

  // Updates thumb_ below.
  void UpdateThumbState(const HardwareState& hwstate);

  // Returns true iff the keyboard has been recently used.
  bool KeyboardRecentlyUsed(stime_t now) const;

  // Updates non_gs_fingers based on a new hardware state. Removes missing and
  // newly moving fingers from non_gs_fingers.
  void UpdateNonGsFingers(const HardwareState& hwstate);

  // Gets the finger or fingers we should consider for gestures.
  // Currently, it fetches the (up to) two fingers closest to the keyboard
  // that are not palms. There is one exception: for t5r2 pads with > 2
  // fingers present, we return all fingers.
  FingerMap GetGesturingFingers(const HardwareState& hwstate) const;

  // Updates current_gesture_type_ based on passed-in hwstate and
  // considering the passed in fingers as gesturing.
  // Returns the finger(s) that are performing the gesture in
  // active_gs_fingers.
  void UpdateCurrentGestureType(const HardwareState& hwstate,
                                const FingerMap& gs_fingers,
                                FingerMap* active_gs_fingers);

  // Sorts the fingers referred to in finger_ids (whose details are in hwstate)
  // according to prodimity and places the sorted range into out_sorted_ids.
  // The sort first finds the two closes points and includes them first.
  // Then, it finds the point closest to any included point, repeating until
  // all points are included.
  static void SortFingersByProximity(
      const FingerMap& finger_ids,
      const HardwareState& hwstate,
      vector<short, kMaxGesturingFingers>* out_sorted_ids);

  // If the finger is likely to be a palm and that its contact size/pressure
  // is diminishing/increasing, we suppress the cursor movement. A real
  // intentional 1f cursor move near the touchpad boundary usually has a
  // stationary finger contact size/pressure.
  bool PalmIsArrivingOrDeparting(const FingerState& finger) const;

  // Check if a finger is close to any known thumb. Can be used to detect some
  // thumb split cases.
  bool IsTooCloseToThumb(const FingerState& finger) const;

  // If the fingers are near each other in location and pressure and might
  // to be part of a 2-finger action, returns true. The function can also
  // be used to check whether the gesture is a left or a right button click
  // with the parameter checking_button_type.
  bool TwoFingersGesturing(const FingerState& finger1,
                           const FingerState& finger2,
                           bool check_button_type) const;

  // Given that TwoFingersGesturing returns true for 2 fingers,
  // This will further look to see if it's really 2 finger scroll or not.
  // Returns the current state (move or scroll) or kGestureTypeNull if
  // unknown.
  GestureType GetTwoFingerGestureType(const FingerState& finger1,
                                      const FingerState& finger2);

  // Check for a pinch gesture and update the state machine for detection.
  // If a pinch was detected it will return true. False otherwise.
  // To reset the state machine call with reset=true
  bool UpdatePinchState(const HardwareState& hwstate, bool reset,
                        const FingerMap& gs_fingers);

  // Returns a gesture assuming that at least one of the fingers performing
  // current_gesture_type has left
  GestureType GetFingerLiftGesture(GestureType current_gesture_type);

  // Returns the current multi-finger gesture, or kGestureTypeNull if no gesture
  // should be produced. num_fingers can be 3 or 4.
  GestureType GetMultiFingerGestureType(const FingerState* const fingers[],
                                        const int num_fingers);

  const char* TapToClickStateName(TapToClickState state);

  stime_t TimeoutForTtcState(TapToClickState state);

  void SetTapToClickState(TapToClickState state,
                          stime_t now);

  void UpdateTapGesture(const HardwareState* hwstate,
                        const FingerMap& gs_fingers,
                        const bool same_fingers,
                        stime_t now,
                        stime_t* timeout);

  void UpdateTapState(const HardwareState* hwstate,
                      const FingerMap& gs_fingers,
                      const bool same_fingers,
                      stime_t now,
                      unsigned* buttons_down,
                      unsigned* buttons_up,
                      stime_t* timeout);

  // Returns true iff the given finger is too close to any other finger to
  // realistically be doing a tap gesture.
  bool FingerTooCloseToTap(const HardwareState& hwstate, const FingerState& fs);

  // Returns true iff finger is in the bottom, dampened zone of the pad
  bool FingerInDampenedZone(const FingerState& finger) const;

  // Called when fingers have changed to fill start_positions_
  // and origin_positions_.
  void FillStartPositions(const HardwareState& hwstate);

  // Fills the origin_* member variables.
  void FillOriginInfo(const HardwareState& hwstate);

  // Fills moving_ with any moving fingers.
  FingerMap UpdateMovingFingers(const HardwareState& hwstate);

  // Update started_moving_time_ to now if any gesturing fingers started moving.
  void UpdateStartedMovingTime(stime_t now,
                               const FingerMap& gs_fingers,
                               const FingerMap& newly_moving_fingers);

  // Updates the internal button state based on the passed in |hwstate|.
  // Can optionally request a timeout by setting *timeout.
  void UpdateButtons(const HardwareState& hwstate, stime_t* timeout);

  // Called when the timeout is fired for UpdateButtons.
  void UpdateButtonsTimeout(stime_t now);

  // By looking at |hwstate| and internal state, determins if a button down
  // at this time would correspond to a left/middle/right click. Returns
  // GESTURES_BUTTON_{LEFT,MIDDLE,RIGHT}.
  int EvaluateButtonType(const HardwareState& hwstate,
                         stime_t button_down_time);

  // Precondition: current_mode_ is set to the mode based on |hwstate|.
  // Computes the resulting gesture, storing it in result_.
  void FillResultGesture(const HardwareState& hwstate,
                         const FingerMap& fingers);

  virtual void IntWasWritten(IntProperty* prop);

  // Fingers which are prohibited from ever tapping.
  set<short, kMaxFingers> tap_dead_fingers_;

  // Active gs fingers are the subset of gs_fingers that are actually performing
  // a gesture
  FingerMap prev_active_gs_fingers_;

  // Fingers that would be considered as possibly gesturing, but others fingers
  // did the gesturing.
  FingerMap non_gs_fingers_;

  FingerMap prev_gs_fingers_;
  FingerMap prev_tap_gs_fingers_;
  HardwareProperties hw_props_;
  Gesture result_;
  Gesture prev_result_;

  // Time when a contact arrived. Persists even when fingers change.
  map<short, stime_t, kMaxFingers> origin_timestamps_;

  // Total distance travelled by a finger since the origin_timestamps_.
  map<short, float, kMaxFingers> distance_walked_;

  // Button data
  // Which button we are going to send/have sent for the physical btn press
  int button_type_;  // left, middle, or right

  FingerButtonClick finger_button_click_;

  // If we have sent button down for the currently down button
  bool sent_button_down_;

  // If we haven't sent a button down by this time, send one
  stime_t button_down_timeout_;

  // When fingers change, we record the time
  stime_t changed_time_;

  // When gesturing fingers move after change, we record the time.
  stime_t started_moving_time_;
  // Record which fingers have started moving already.
  set<short, kMaxFingers> moving_;

  // When different fingers are gesturing, we record the time
  stime_t gs_changed_time_;

  // When fingers leave, we record the time
  stime_t finger_leave_time_;

  // When fingers change, we keep track of where they started.
  // Map: Finger ID -> (x, y) coordinate
  map<short, Point, kMaxFingers> start_positions_;

  // We keep track of where each finger started when they touched.
  // Map: Finger ID -> (x, y) coordinate.
  map<short, Point, kMaxFingers> origin_positions_;

  // tracking ids of known fingers that are not palms, nor thumbs.
  set<short, kMaxFingers> pointing_;
  // tracking ids of known non-palms. But might be thumbs.
  set<short, kMaxFingers> fingers_;
  // contacts believed to be thumbs, and when they were inserted into the map
  map<short, stime_t, kMaxFingers> thumb_;
  // Timer of the evaluation period for contacts believed to be thumbs.
  map<short, stime_t, kMaxFingers> thumb_eval_timer_;

  // once a moving finger is determined lock onto this one for cursor movement.
  short moving_finger_id_;

  // Tap-to-click
  // The current state:
  TapToClickState tap_to_click_state_;

  // When we entered the state:
  stime_t tap_to_click_state_entered_;

  TapRecord tap_record_;

  // Record time when the finger showed motion (uses different motion detection
  // than last_movement_timestamp_)
  stime_t tap_drag_last_motion_time_;

  // True when the finger was stationary for a while during tap to drag
  bool tap_drag_finger_was_stationary_;

  // Time when the last motion (scroll, movement) occurred
  stime_t last_movement_timestamp_;

  // Time when the last swipe gesture was generated
  stime_t last_swipe_timestamp_;
  bool swipe_is_vertical_;

  // If we are currently pointing, scrolling, etc.
  GestureType current_gesture_type_;
  // Previous value of current_gesture_type_
  GestureType prev_gesture_type_;

  // Cache for distance between fingers at previous pinch gesture event, or
  // start of pinch detection
  float pinch_prev_distance_sq_;

  HardwareStateBuffer state_buffer_;
  ScrollEventBuffer scroll_buffer_;

  FingerMetrics* finger_metrics_;
  std::unique_ptr<FingerMetrics> test_finger_metrics_;

  // There are three pinch guess states before locking:
  //   pinch_guess_start_ == -1: No definite guess made about pinch
  //   pinch_guess_start_ > 0:
  //     pinch_guess_ == true:  Guess there is a pinch
  //     pinch_guess_ == false: Guess there is no pinch

  // Since the fingers changed, has a scroll or swipe gesture been detected?
  bool these_fingers_scrolled_;
  // When guessing a pinch gesture. Do we guess pinch (true) or no-pinch?
  bool pinch_guess_;
  // Time when pinch guess was made. -1 if no guess has been made yet.
  stime_t pinch_guess_start_;
  // True when the pinch decision has been locked.
  bool pinch_locked_;
  // Pinch status: GESTURES_ZOOM_START, _UPDATE, or _END
  unsigned pinch_status_;
  // Direction of previous pinch update:
  //   0: No previous update
  //   1: Outward
  //  -1: Inward
  int pinch_prev_direction_;
  // Timestamp of previous pinch update
  float pinch_prev_time_;

  // Keeps track of if there was a finger seen during a physical click
  bool finger_seen_shortly_after_button_down_;

  ScrollManager scroll_manager_;

  // Properties

  // Is Tap-To-Click enabled
  BoolProperty tap_enable_;
  // Allows Tap-To-Click to be paused
  BoolProperty tap_paused_;
  // General time limit [s] for tap gestures
  DoubleProperty tap_timeout_;
  // General time limit [s] for time between taps.
  DoubleProperty inter_tap_timeout_;
  // Time [s] before a tap gets recognized as a drag.
  DoubleProperty tap_drag_delay_;
  // Time [s] it takes to stop dragging when you let go of the touchpad
  DoubleProperty tap_drag_timeout_;
  // True if tap dragging is enabled. With it disbled we can respond quickly
  // to tap clicks.
  BoolProperty tap_drag_enable_;
  // True if drag lock is enabled
  BoolProperty drag_lock_enable_;
  // Time [s] the finger has to be stationary to be considered dragging
  DoubleProperty tap_drag_stationary_time_;
  // Distance [mm] a finger can move and still register a tap
  DoubleProperty tap_move_dist_;
  // Minimum pressure a finger must have for it to click when tap to click is on
  DoubleProperty tap_min_pressure_;
  // Maximum distance [mm] per frame that a finger can move and still be
  // considered stationary.
  DoubleProperty tap_max_movement_;
  // Maximum finger age for a finger to trigger tap.
  DoubleProperty tap_max_finger_age_;
  // If three finger click should be enabled. This is a temporary flag so that
  // we can deploy this feature behind a file while we work out the bugs.
  BoolProperty three_finger_click_enable_;
  // If zero finger click should be enabled. On some platforms, bending the
  // case may accidentally cause a physical click.  This supresses all clicks
  // that do not have at least 1 finger detected on the touchpad.
  BoolProperty zero_finger_click_enable_;
  // If T5R2 should support three-finger click/tap, which can in some situations
  // be unreliable.
  BoolProperty t5r2_three_finger_click_enable_;
  // Distance [mm] a finger must move after fingers change to count as real
  // motion
  DoubleProperty change_move_distance_;
  // Speed [mm/s] a finger must move to lock on to that finger
  DoubleProperty move_lock_speed_;
  // Distance [mm] a finger must move to report that movement
  DoubleProperty move_report_distance_;
  // Time [s] to block movement after number or identify of fingers change
  DoubleProperty change_timeout_;
  // Time [s] to wait before locking on to a gesture
  DoubleProperty evaluation_timeout_;
  // Time [s] to wait before deciding if the pinch zoom is happening.
  DoubleProperty pinch_evaluation_timeout_;
  // Time [s] to wait before decide if a thumb is doing a pinch
  DoubleProperty thumb_pinch_evaluation_timeout_;
  // Minimum movement that a thumb should have to be a gesturing finger.
  DoubleProperty thumb_pinch_min_movement_;
  // If the ratio of gesturing fingers movement to thumb movement is greater
  // than this threshold, then we might detect a slow pinch.
  // The movements are compared by Distance_sq * Time * Time
  DoubleProperty slow_pinch_guess_ratio_threshold_;
  // If the ratio of gesturing fingers movement to thumb movement is greater
  // than this number, then we can't have pinch with thumb.
  DoubleProperty thumb_pinch_movement_ratio_;
  // Ratio of Distance_sq * Time * Time for two fingers. This measure is used
  // to compare the slow movement of two fingers.
  DoubleProperty thumb_slow_pinch_similarity_ratio_;
  // If a thumb arrives at the same time as the other fingers, the
  // thumb_pinch_evaluation_timeout_ is multiplied by this factor
  DoubleProperty thumb_pinch_delay_factor_;
  // Minimum movement that fingers must have before we consider their
  // relative direction. If the movement is smaller than this number, it
  // will considered as noise.
  DoubleProperty minimum_movement_direction_detection_;
  // A finger in the damp zone must move at least this much as much as
  // the other finger to count toward a gesture. Should be between 0 and 1.
  DoubleProperty damp_scroll_min_movement_factor_;
  // If two fingers have a pressure difference greater than diff thresh and
  // the larger is more than diff factor times the smaller, we assume the
  // larger is a thumb.
  DoubleProperty two_finger_pressure_diff_thresh_;
  DoubleProperty two_finger_pressure_diff_factor_;
  // Click-and-drags are sometimes wrongly classified as right-clicks if the
  // physical-clicking finger arrives at the pad later than or at roughly the
  // same time of the other finger. To distinguish between the two cases, we
  // use the pressure difference and the fingers' relative positions.
  DoubleProperty click_drag_pressure_diff_thresh_;
  DoubleProperty click_drag_pressure_diff_factor_;
  // Mininum slope of the line connecting two fingers that can qualify a click-
  // and-drag gesture.
  DoubleProperty click_drag_min_slope_;
  // If a large contact moves more than this much times the lowest-pressure
  // contact, consider it not to be a thumb.
  DoubleProperty thumb_movement_factor_;
  // If a large contact moves faster than this much times the lowest-pressure
  // contact, consider it not to be a thumb.
  DoubleProperty thumb_speed_factor_;
  // This much time after fingers change, stop allowing contacts classified
  // as thumb to be classified as non-thumb.
  DoubleProperty thumb_eval_timeout_;
  // If thumb is doing an inward pinch, the thresholds for distance and speed
  // that thumb needs to move to be a gesturing finger are multiplied by this
  // factor
  DoubleProperty thumb_pinch_threshold_ratio_;
  // If a finger is recognized as thumb, it has only this much time to change
  // its status and perform a click
  DoubleProperty thumb_click_prevention_timeout_;
  // Consider scroll vs pointing if finger moves at least this distance [mm]
  DoubleProperty two_finger_scroll_distance_thresh_;
  // Consider move if there is no scroll and one finger moves at least this
  // distance [mm]
  DoubleProperty two_finger_move_distance_thresh_;
  // Maximum distance [mm] between the outermost fingers while performing a
  // three-finger gesture.
  DoubleProperty three_finger_close_distance_thresh_;
  // Maximum distance [mm] between the outermost fingers while performing a
  // four-finger gesture.
  DoubleProperty four_finger_close_distance_thresh_;
  // Minimum distance [mm] one of the three fingers must move to perform a
  // swipe gesture.
  DoubleProperty three_finger_swipe_distance_thresh_;
  // Minimum distance [mm] one of the four fingers must move to perform a
  // four finger swipe gesture.
  DoubleProperty four_finger_swipe_distance_thresh_;
  // Minimum ratio between least and most moving finger to perform a
  // three finger swipe gesture.
  DoubleProperty three_finger_swipe_distance_ratio_;
  // Minimum ratio between least and most moving finger to perform a
  // four finger swipe gesture.
  DoubleProperty four_finger_swipe_distance_ratio_;
  // If three-finger swipe should be enabled
  BoolProperty three_finger_swipe_enable_;
  // Height [mm] of the bottom zone
  DoubleProperty bottom_zone_size_;
  // Time [s] to after button down to evaluate number of fingers for a click
  DoubleProperty button_evaluation_timeout_;
  // Time [s] to evaluate number of fingers for a click after a new touch has
  // been registered
  DoubleProperty button_finger_timeout_;
  // Distance [mm] a finger can move to still be considered for a button click
  DoubleProperty button_move_dist_;
  // Distance [mm] a finger can be away from it's expected location to be
  // considered part of the same finger group
  DoubleProperty button_max_dist_from_expected_;
  // Flag to enable the right click on the right side of the hardware button
  BoolProperty button_right_click_zone_enable_;
  // The size of the right click zone on the right side of the hardware button
  DoubleProperty button_right_click_zone_size_;
  // Timeval of time when keyboard was last touched. After the low one is set,
  // the two are converted into an stime_t and stored in keyboard_touched_.
  IntProperty keyboard_touched_timeval_high_;  // seconds
  IntProperty keyboard_touched_timeval_low_;  // microseconds
  stime_t keyboard_touched_;
  // During this timeout, which is time [s] since the keyboard has been used,
  // we are extra aggressive in palm detection. If this time is > 10s apart
  // from now (either before or after), it's disregarded. We disregard old
  // values b/c they no longer apply. Because of delays in other interpreters
  // (LooaheadInterpreter), it's possible to get "future" keyboard used times.
  // We wouldn't want a single bad future value to stop all tap-to-click, so
  // we sanity check.
  DoubleProperty keyboard_palm_prevent_timeout_;
  // Motion (pointer movement, scroll) must halt for this length of time [s]
  // before a tap can generate a click.
  DoubleProperty motion_tap_prevent_timeout_;
  // A finger must be at least this far from other fingers when it taps [mm].
  DoubleProperty tapping_finger_min_separation_;

  // Ratio between finger movement that indicates not-a-pinch gesture
  DoubleProperty no_pinch_guess_ratio_;
  // Ratio between finger movement that certainly indicates not-a-pinch gesture
  DoubleProperty no_pinch_certain_ratio_;
  // Sum of squares of movement [mm] that is considered as noise during pinch
  // detection
  DoubleProperty pinch_noise_level_sq_;
  // Minimal distance [mm] fingers have to move to indicate a pinch gesture.
  DoubleProperty pinch_guess_min_movement_;
  // Minimal distance [mm] a thumb have to move to do a pinch gesture.
  DoubleProperty pinch_thumb_min_movement_;
  // Minimal distance [mm] fingers have to move to lock a pinch gesture.
  DoubleProperty pinch_certain_min_movement_;
  // Minimum Cos(A) that is acceptable for an inward pinch zoom, where A
  // is the angle between the lower finger and a vertical vector directed
  // from top to bottom.
  DoubleProperty inward_pinch_min_angle_;
  // Maximum Cos(A) to perform a pinch zoom, where A is the angle between
  // two fingers.
  DoubleProperty pinch_zoom_max_angle_;
  // Minimum Cos(A) to perform a scroll gesture when pinch is enabled,
  // where A is the angle between two fingers.
  DoubleProperty scroll_min_angle_;
  // Minimum movement ratio between fingers before we call it a consistent move
  // for a pinch.
  DoubleProperty pinch_guess_consistent_mov_ratio_;
  // Minimum number of touch events needed to start a pinch zoom
  IntProperty pinch_zoom_min_events_;
  // If a pinch is determined quickly we use the original landing position to
  // determing original pinch width. But if they landed too long ago we use the
  // pinch width at detection. Inverse of time in seconds.
  DoubleProperty pinch_initial_scale_time_inv_;
  // Resolution of pinch events: minimum change in squared pinch scale required
  // to send a pinch update.
  DoubleProperty pinch_res_;
  // Change in squared pinch scale required to send a pinch update after fingers
  // stay stationary.
  DoubleProperty pinch_stationary_res_;
  // Time fingers should remain motionless before being treated as stationary.
  DoubleProperty pinch_stationary_time_;
  // Change in squared pinch scale required to send a pinch update after fingers
  // change direction.
  DoubleProperty pinch_hysteresis_res_;
  // Temporary flag to turn pinch on/off while we tune it.
  BoolProperty pinch_enable_;

  // Short start time diff of fingers for a two-finger click that indicates
  // a right click
  DoubleProperty right_click_start_time_diff_;
  // Second finger comes down for a while then button clicks down that indicates
  // a right click
  DoubleProperty right_click_second_finger_age_;
  // Suppress moves with a speed more than this much times the previous speed.
  DoubleProperty quick_acceleration_factor_;
};

bool AnyGesturingFingerLeft(const HardwareState& state,
                            const FingerMap& prev_gs_fingers);

}  // namespace gestures

#endif  // GESTURES_IMMEDIATE_INTERPRETER_H_
