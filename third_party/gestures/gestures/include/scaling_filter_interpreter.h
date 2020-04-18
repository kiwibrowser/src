// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_SCALING_FILTER_INTERPRETER_H_
#define GESTURES_SCALING_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter scales both incoming hardware state and outgoing gesture
// objects to make it easier for library code to do interpretation work.

// Incoming hardware events are in the units of touchpad pixels, which may
// not be square. We convert these to a same-sized touchpad such that
// the units are mm with a (0,0) origin. Correspondingly, we convert the
// gestures from mm units to screen pixels.

// For example, say we have a touchpad that has these properties:
// size: 100m x 60mm, left/right: 133/10279, top/bottom: 728/5822.
// This class will scale/translate it, so that the next Interpreter is told
// the hardware has these properties:
// size: 100m x 60mm, left/right: 0.0/100.0, top/bottom: 0.0/60.0.
// Incoming hardware states will be scaled in transit.

// Also, the screen DPI will be scaled, so that it exactly matches the
// touchpad, by having 1 dot per mm. Thus, the screen DPI told to the next
// Interpreter will be 25.4.
// Outgoing gesture objects will be scaled in transit to what the screen
// actually uses.

// This interpreter can be configured to compute surface area in square mm
// from pressure data or from touch major and minor, as some hardware prefers
// reporting pressure data but some other touch major and minor.
//
// When the pressure is converted (based on properties) to surface area, the
// two properties allow a configuration file to specify a linear relationship
// between pressure and surface area.

class ScalingFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(ScalingFilterInterpreterTest, SimpleTest);
  FRIEND_TEST(ScalingFilterInterpreterTest, TouchMajorAndMinorTest);
 public:
  // Takes ownership of |next|:
  ScalingFilterInterpreter(PropRegistry* prop_reg,
                           Interpreter* next,
                           Tracer* tracer,
                           GestureInterpreterDeviceClass devclass);
  virtual ~ScalingFilterInterpreter() {}

  virtual void Initialize(const HardwareProperties* hwprops,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);
 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  void ScaleHardwareState(HardwareState* hwstate);
  void ScaleMouseHardwareState(HardwareState* hwstate);
  void ScaleTouchpadHardwareState(HardwareState* hwstate);
  void ConsumeGesture(const Gesture& gs);
  void FilterLowPressure(HardwareState* hwstate);
  void FilterZeroArea(HardwareState* hwstate);
  bool IsMouseDevice(GestureInterpreterDeviceClass devclass);
  bool IsTouchpadDevice(GestureInterpreterDeviceClass devclass);

  float tp_x_scale_, tp_y_scale_;
  float tp_x_translate_, tp_y_translate_;

  float screen_x_scale_, screen_y_scale_;

  // When orientation_scale_ = 0, no orientation is provided from kernel.
  float orientation_scale_;

  // True if scrolling should be inverted
  BoolProperty australian_scrolling_;

  // Output surface area (sq. mm) =
  // if surface_area_from_pressure_
  //   input pressure * pressure_scale_ + pressure_translate_
  // else
  //   if input touch_major != 0 and input touch_minor != 0
  //     pi / 4 * output touch_major * output touch_minor
  //   else if input touch_major != 0
  //     pi / 4 * output touch_major^2
  //   else
  //     0
  BoolProperty surface_area_from_pressure_;

  // Touchpad device output bias (pixels).
  DoubleProperty tp_x_bias_;
  DoubleProperty tp_y_bias_;

  DoubleProperty pressure_scale_;
  DoubleProperty pressure_translate_;
  DoubleProperty pressure_threshold_;
  // if true, the low pressure touch will be ignored.
  // if false, or doesn't exist, the low pressure touch will be converted
  // to touch with pressure 1.0
  BoolProperty filter_low_pressure_;

  // If true, adjust touch count to match finger count when scaling
  // input state. This can help avoid being considered a T5R2 pad.
  BoolProperty force_touch_count_to_match_finger_count_;

  DoubleProperty mouse_cpi_;

  HardwareProperties friendly_props_;

  // XInput properties that we use to identify the device type in Chrome for
  // all CMT devices. We put them here for now because they are not large
  // enough to constitute a stand-alone class.
  // TODO(sheckylin): Find a better place for them.

  // If the device is mouse. Note that a device can both be a mouse and a
  // touchpad at the same time (e.g. a multi-touch mouse).
  BoolProperty device_mouse_;

  // If the device is touchpad. It would be false if it is a regular mouse
  // running the CMT driver.
  BoolProperty device_touchpad_;
};

}  // namespace gestures

#endif  // GESTURES_SCALING_FILTER_INTERPRETER_H_
