// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/activity_log.h"

#include <errno.h>
#include <fcntl.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include <json/value.h>
#include <json/writer.h>

#include "gestures/include/file_util.h"
#include "gestures/include/logging.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/string_util.h"

// This should be set by build system:
#ifndef VCSID
#define VCSID "Unknown"
#endif  // VCSID

#define QUINTTAP_COUNT 5  /* BTN_TOOL_QUINTTAP - Five fingers on trackpad */

using std::set;
using std::string;

namespace gestures {

ActivityLog::ActivityLog(PropRegistry* prop_reg)
    : head_idx_(0), size_(0), max_fingers_(0), hwprops_(),
      prop_reg_(prop_reg) {}

void ActivityLog::SetHardwareProperties(const HardwareProperties& hwprops) {
  hwprops_ = hwprops;

  // For old devices(such as mario, alex, zgb..), the reporting touch count
  // or 'max_touch_cnt' will be less than number of slots or 'max_finger_cnt'
  // they support. As kernel evdev drivers do not have a bitset to report
  // touch count greater than five (bitset for five-fingers gesture is
  // BTN_TOOL_QUINTAP), we respect 'max_finger_cnt' than 'max_touch_cnt'
  // reported from kernel driver as the 'max_fingers_' instead.
  if (hwprops.max_touch_cnt < QUINTTAP_COUNT) {
    max_fingers_ = std::min<size_t>(hwprops.max_finger_cnt,
                                    hwprops.max_touch_cnt);
  } else {
    max_fingers_ = std::max<size_t>(hwprops.max_finger_cnt,
                                    hwprops.max_touch_cnt);
  }

  finger_states_.reset(new FingerState[kBufferSize * max_fingers_]);
}

void ActivityLog::LogHardwareState(const HardwareState& hwstate) {
  Entry* entry = PushBack();
  entry->type = kHardwareState;
  entry->details.hwstate = hwstate;
  if (hwstate.finger_cnt > max_fingers_) {
    Err("Too many fingers! Max is %zu, but I got %d",
        max_fingers_, hwstate.finger_cnt);
    entry->details.hwstate.fingers = NULL;
    entry->details.hwstate.finger_cnt = 0;
    return;
  }
  if (!finger_states_.get())
    return;
  entry->details.hwstate.fingers = &finger_states_[TailIdx() * max_fingers_];
  std::copy(&hwstate.fingers[0], &hwstate.fingers[hwstate.finger_cnt],
            entry->details.hwstate.fingers);
}

void ActivityLog::LogTimerCallback(stime_t now) {
  Entry* entry = PushBack();
  entry->type = kTimerCallback;
  entry->details.timestamp = now;
}

void ActivityLog::LogCallbackRequest(stime_t when) {
  Entry* entry = PushBack();
  entry->type = kCallbackRequest;
  entry->details.timestamp = when;
}

void ActivityLog::LogGesture(const Gesture& gesture) {
  Entry* entry = PushBack();
  entry->type = kGesture;
  entry->details.gesture = gesture;
}

void ActivityLog::LogPropChange(const PropChangeEntry& prop_change) {
  Entry* entry = PushBack();
  entry->type = kPropChange;
  entry->details.prop_change = prop_change;
}

void ActivityLog::Dump(const char* filename) {
  string data = Encode();
  WriteFile(filename, data.c_str(), data.size());
}

ActivityLog::Entry* ActivityLog::PushBack() {
  if (size_ == kBufferSize) {
    Entry* ret = &buffer_[head_idx_];
    head_idx_ = (head_idx_ + 1) % kBufferSize;
    return ret;
  }
  ++size_;
  return &buffer_[TailIdx()];
}

Json::Value ActivityLog::EncodeHardwareProperties() const {
  Json::Value ret(Json::objectValue);
  ret[kKeyHardwarePropLeft] = Json::Value(hwprops_.left);
  ret[kKeyHardwarePropTop] = Json::Value(hwprops_.top);
  ret[kKeyHardwarePropRight] = Json::Value(hwprops_.right);
  ret[kKeyHardwarePropBottom] = Json::Value(hwprops_.bottom);
  ret[kKeyHardwarePropXResolution] = Json::Value(hwprops_.res_x);
  ret[kKeyHardwarePropYResolution] = Json::Value(hwprops_.res_y);
  ret[kKeyHardwarePropXDpi] = Json::Value(hwprops_.screen_x_dpi);
  ret[kKeyHardwarePropYDpi] = Json::Value(hwprops_.screen_y_dpi);
  ret[kKeyHardwarePropOrientationMinimum] =
      Json::Value(hwprops_.orientation_minimum);
  ret[kKeyHardwarePropOrientationMaximum] =
      Json::Value(hwprops_.orientation_maximum);
  ret[kKeyHardwarePropMaxFingerCount] =
      Json::Value(hwprops_.max_finger_cnt);
  ret[kKeyHardwarePropMaxTouchCount] =
      Json::Value(hwprops_.max_touch_cnt);

  ret[kKeyHardwarePropSupportsT5R2] =
      Json::Value(hwprops_.supports_t5r2 != 0);
  ret[kKeyHardwarePropSemiMt] =
      Json::Value(hwprops_.support_semi_mt != 0);
  ret[kKeyHardwarePropIsButtonPad] =
      Json::Value(hwprops_.is_button_pad != 0);
  ret[kKeyHardwarePropHasWheel] =
      Json::Value(hwprops_.has_wheel != 0);
  return ret;
}

Json::Value ActivityLog::EncodeHardwareState(const HardwareState& hwstate) {
  Json::Value ret(Json::objectValue);
  ret[kKeyType] = Json::Value(kKeyHardwareState);
  ret[kKeyHardwareStateButtonsDown] =
      Json::Value(hwstate.buttons_down);
  ret[kKeyHardwareStateTouchCnt] =
      Json::Value(hwstate.touch_cnt);
  ret[kKeyHardwareStateTimestamp] =
      Json::Value(hwstate.timestamp);
  Json::Value fingers(Json::arrayValue);
  for (size_t i = 0; i < hwstate.finger_cnt; ++i) {
    if (hwstate.fingers == NULL) {
      Err("Have finger_cnt %d but fingers is NULL!", hwstate.finger_cnt);
      break;
    }
    const FingerState& fs = hwstate.fingers[i];
    Json::Value finger(Json::objectValue);
    finger[kKeyFingerStateTouchMajor] =
        Json::Value(fs.touch_major);
    finger[kKeyFingerStateTouchMinor] =
        Json::Value(fs.touch_minor);
    finger[kKeyFingerStateWidthMajor] =
        Json::Value(fs.width_major);
    finger[kKeyFingerStateWidthMinor] =
        Json::Value(fs.width_minor);
    finger[kKeyFingerStatePressure] =
        Json::Value(fs.pressure);
    finger[kKeyFingerStateOrientation] =
        Json::Value(fs.orientation);
    finger[kKeyFingerStatePositionX] =
        Json::Value(fs.position_x);
    finger[kKeyFingerStatePositionY] =
        Json::Value(fs.position_y);
    finger[kKeyFingerStateTrackingId] =
        Json::Value(fs.tracking_id);
    finger[kKeyFingerStateFlags] =
        Json::Value(static_cast<int>(fs.flags));
    fingers.append(finger);
  }
  ret[kKeyHardwareStateFingers] = fingers;
  ret[kKeyHardwareStateRelX] =
      Json::Value(hwstate.rel_x);
  ret[kKeyHardwareStateRelY] =
      Json::Value(hwstate.rel_y);
  ret[kKeyHardwareStateRelWheel] =
      Json::Value(hwstate.rel_wheel);
  ret[kKeyHardwareStateRelHWheel] =
      Json::Value(hwstate.rel_hwheel);
  return ret;
}

Json::Value ActivityLog::EncodeTimerCallback(stime_t timestamp) {
  Json::Value ret(Json::objectValue);
  ret[kKeyType] = Json::Value(kKeyTimerCallback);
  ret[kKeyTimerCallbackNow] = Json::Value(timestamp);
  return ret;
}

Json::Value ActivityLog::EncodeCallbackRequest(stime_t timestamp) {
  Json::Value ret(Json::objectValue);
  ret[kKeyType] = Json::Value(kKeyCallbackRequest);
  ret[kKeyCallbackRequestWhen] = Json::Value(timestamp);
  return ret;
}

Json::Value ActivityLog::EncodeGesture(const Gesture& gesture) {
  Json::Value ret(Json::objectValue);
  ret[kKeyType] = Json::Value(kKeyGesture);
  ret[kKeyGestureStartTime] = Json::Value(gesture.start_time);
  ret[kKeyGestureEndTime] = Json::Value(gesture.end_time);

  bool handled = false;
  switch (gesture.type) {
    case kGestureTypeNull:
      handled = true;
      ret[kKeyGestureType] = Json::Value("null");
      break;
    case kGestureTypeContactInitiated:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeContactInitiated);
      break;
    case kGestureTypeMove:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeMove);
      ret[kKeyGestureMoveDX] =
          Json::Value(gesture.details.move.dx);
      ret[kKeyGestureMoveDY] =
          Json::Value(gesture.details.move.dy);
      ret[kKeyGestureMoveOrdinalDX] =
          Json::Value(gesture.details.move.ordinal_dx);
      ret[kKeyGestureMoveOrdinalDY] =
          Json::Value(gesture.details.move.ordinal_dy);
      break;
    case kGestureTypeScroll:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeScroll);
      ret[kKeyGestureScrollDX] =
          Json::Value(gesture.details.scroll.dx);
      ret[kKeyGestureScrollDY] =
          Json::Value(gesture.details.scroll.dy);
      ret[kKeyGestureScrollOrdinalDX] =
          Json::Value(gesture.details.scroll.ordinal_dx);
      ret[kKeyGestureScrollOrdinalDY] =
          Json::Value(gesture.details.scroll.ordinal_dy);
      break;
    case kGestureTypePinch:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypePinch);
      ret[kKeyGesturePinchDZ] =
          Json::Value(gesture.details.pinch.dz);
      ret[kKeyGesturePinchOrdinalDZ] =
          Json::Value(gesture.details.pinch.ordinal_dz);
      ret[kKeyGesturePinchZoomState] =
          Json::Value(gesture.details.pinch.zoom_state);
      break;
    case kGestureTypeButtonsChange:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeButtonsChange);
      ret[kKeyGestureButtonsChangeDown] =
          Json::Value(
                   static_cast<int>(gesture.details.buttons.down));
      ret[kKeyGestureButtonsChangeUp] =
          Json::Value(
                   static_cast<int>(gesture.details.buttons.up));
      break;
    case kGestureTypeFling:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeFling);
      ret[kKeyGestureFlingVX] =
          Json::Value(gesture.details.fling.vx);
      ret[kKeyGestureFlingVY] =
          Json::Value(gesture.details.fling.vy);
      ret[kKeyGestureFlingOrdinalVX] =
          Json::Value(gesture.details.fling.ordinal_vx);
      ret[kKeyGestureFlingOrdinalVY] =
          Json::Value(gesture.details.fling.ordinal_vy);
      ret[kKeyGestureFlingState] =
          Json::Value(
                   static_cast<int>(gesture.details.fling.fling_state));
      break;
    case kGestureTypeSwipe:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeSwipe);
      ret[kKeyGestureSwipeDX] =
          Json::Value(gesture.details.swipe.dx);
      ret[kKeyGestureSwipeDY] =
          Json::Value(gesture.details.swipe.dy);
      ret[kKeyGestureSwipeOrdinalDX] =
          Json::Value(gesture.details.swipe.ordinal_dx);
      ret[kKeyGestureSwipeOrdinalDY] =
          Json::Value(gesture.details.swipe.ordinal_dy);
      break;
    case kGestureTypeSwipeLift:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeSwipeLift);
      break;
    case kGestureTypeFourFingerSwipe:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeFourFingerSwipe);
      ret[kKeyGestureFourFingerSwipeDX] =
          Json::Value(gesture.details.four_finger_swipe.dx);
      ret[kKeyGestureFourFingerSwipeDY] =
          Json::Value(gesture.details.four_finger_swipe.dy);
      ret[kKeyGestureFourFingerSwipeOrdinalDX] =
          Json::Value(gesture.details.four_finger_swipe.ordinal_dx);
      ret[kKeyGestureFourFingerSwipeOrdinalDY] =
          Json::Value(gesture.details.four_finger_swipe.ordinal_dy);
      break;
    case kGestureTypeFourFingerSwipeLift:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeFourFingerSwipeLift);
      break;
    case kGestureTypeMetrics:
      handled = true;
      ret[kKeyGestureType] =
          Json::Value(kValueGestureTypeMetrics);
      ret[kKeyGestureMetricsType] =
          Json::Value(
                   static_cast<int>(gesture.details.metrics.type));
      ret[kKeyGestureMetricsData1] =
          Json::Value(gesture.details.metrics.data[0]);
      ret[kKeyGestureMetricsData2] =
          Json::Value(gesture.details.metrics.data[1]);
      break;
  }
  if (!handled)
    ret[kKeyGestureType] =
        Json::Value(StringPrintf("Unhandled %d", gesture.type));
  return ret;
}

Json::Value ActivityLog::EncodePropChange(const PropChangeEntry& prop_change) {
  Json::Value ret(Json::objectValue);
  ret[kKeyType] = Json::Value(kKeyPropChange);
  ret[kKeyPropChangeName] = Json::Value(prop_change.name);
  Json::Value val;
  Json::Value type;
  switch (prop_change.type) {
    case PropChangeEntry::kBoolProp:
      val = Json::Value(static_cast<bool>(prop_change.value.bool_val));
      type = Json::Value(kValuePropChangeTypeBool);
      break;
    case PropChangeEntry::kDoubleProp:
      val = Json::Value(prop_change.value.double_val);
      type = Json::Value(kValuePropChangeTypeDouble);
      break;
    case PropChangeEntry::kIntProp:
      val = Json::Value(prop_change.value.int_val);
      type = Json::Value(kValuePropChangeTypeInt);
      break;
    case PropChangeEntry::kShortProp:
      val = Json::Value(prop_change.value.short_val);
      type = Json::Value(kValuePropChangeTypeShort);
      break;
  }
  if (!val.isNull())
    ret[kKeyPropChangeValue] = val;
  if (!type.isNull())
    ret[kKeyPropChangeType] = type;
  return ret;
}

Json::Value ActivityLog::EncodePropRegistry() {
  Json::Value ret(Json::objectValue);
  if (!prop_reg_)
    return ret;

  const set<Property*>& props = prop_reg_->props();
  for (set<Property*>::const_iterator it = props.begin(), e = props.end();
       it != e; ++it) {
    ret[(*it)->name()] = (*it)->NewValue();
  }
  return ret;
}

Json::Value ActivityLog::EncodeCommonInfo() {
  Json::Value root(Json::objectValue);

  Json::Value entries(Json::arrayValue);
  for (size_t i = 0; i < size_; ++i) {
    const Entry& entry = buffer_[(i + head_idx_) % kBufferSize];
    switch (entry.type) {
      case kHardwareState:
        entries.append(EncodeHardwareState(entry.details.hwstate));
        continue;
      case kTimerCallback:
        entries.append(EncodeTimerCallback(entry.details.timestamp));
        continue;
      case kCallbackRequest:
        entries.append(EncodeCallbackRequest(entry.details.timestamp));
        continue;
      case kGesture:
        entries.append(EncodeGesture(entry.details.gesture));
        continue;
      case kPropChange:
        entries.append(EncodePropChange(entry.details.prop_change));
        continue;
    }
    Err("Unknown entry type %d", entry.type);
  }
  root[kKeyRoot] = entries;
  root[kKeyHardwarePropRoot] = EncodeHardwareProperties();

  return root;
}

void ActivityLog::AddEncodeInfo(Json::Value* root) {
  (*root)["version"] = Json::Value(1);
  string gestures_version = VCSID;

  // Strip tailing whitespace.
  TrimWhitespaceASCII(gestures_version, TRIM_ALL, &gestures_version);
  (*root)["gesturesVersion"] = Json::Value(gestures_version);
  (*root)[kKeyProperties] = EncodePropRegistry();
}

string ActivityLog::Encode() {
  Json::Value root = EncodeCommonInfo();
  AddEncodeInfo(&root);
  return root.toStyledString();
}

const char ActivityLog::kKeyInterpreterName[] = "interpreterName";
const char ActivityLog::kKeyNext[] = "nextLayer";
const char ActivityLog::kKeyRoot[] = "entries";
const char ActivityLog::kKeyType[] = "type";
const char ActivityLog::kKeyHardwareState[] = "hardwareState";
const char ActivityLog::kKeyTimerCallback[] = "timerCallback";
const char ActivityLog::kKeyCallbackRequest[] = "callbackRequest";
const char ActivityLog::kKeyGesture[] = "gesture";
const char ActivityLog::kKeyPropChange[] = "propertyChange";
const char ActivityLog::kKeyHardwareStateTimestamp[] = "timestamp";
const char ActivityLog::kKeyHardwareStateButtonsDown[] = "buttonsDown";
const char ActivityLog::kKeyHardwareStateTouchCnt[] = "touchCount";
const char ActivityLog::kKeyHardwareStateFingers[] = "fingers";
const char ActivityLog::kKeyHardwareStateRelX[] = "relX";
const char ActivityLog::kKeyHardwareStateRelY[] = "relY";
const char ActivityLog::kKeyHardwareStateRelWheel[] = "relWheel";
const char ActivityLog::kKeyHardwareStateRelHWheel[] = "relHWheel";
const char ActivityLog::kKeyFingerStateTouchMajor[] = "touchMajor";
const char ActivityLog::kKeyFingerStateTouchMinor[] = "touchMinor";
const char ActivityLog::kKeyFingerStateWidthMajor[] = "widthMajor";
const char ActivityLog::kKeyFingerStateWidthMinor[] = "widthMinor";
const char ActivityLog::kKeyFingerStatePressure[] = "pressure";
const char ActivityLog::kKeyFingerStateOrientation[] = "orientation";
const char ActivityLog::kKeyFingerStatePositionX[] = "positionX";
const char ActivityLog::kKeyFingerStatePositionY[] = "positionY";
const char ActivityLog::kKeyFingerStateTrackingId[] = "trackingId";
const char ActivityLog::kKeyFingerStateFlags[] = "flags";
const char ActivityLog::kKeyTimerCallbackNow[] = "now";
const char ActivityLog::kKeyCallbackRequestWhen[] = "when";
const char ActivityLog::kKeyGestureType[] = "gestureType";
const char ActivityLog::kValueGestureTypeContactInitiated[] =
    "contactInitiated";
const char ActivityLog::kValueGestureTypeMove[] = "move";
const char ActivityLog::kValueGestureTypeScroll[] = "scroll";
const char ActivityLog::kValueGestureTypePinch[] = "pinch";
const char ActivityLog::kValueGestureTypeButtonsChange[] = "buttonsChange";
const char ActivityLog::kValueGestureTypeFling[] = "fling";
const char ActivityLog::kValueGestureTypeSwipe[] = "swipe";
const char ActivityLog::kValueGestureTypeSwipeLift[] = "swipeLift";
const char ActivityLog::kValueGestureTypeFourFingerSwipe[] = "fourFingerSwipe";
const char ActivityLog::kValueGestureTypeFourFingerSwipeLift[] =
    "fourFingerSwipeLift";
const char ActivityLog::kValueGestureTypeMetrics[] = "metrics";
const char ActivityLog::kKeyGestureStartTime[] = "startTime";
const char ActivityLog::kKeyGestureEndTime[] = "endTime";
const char ActivityLog::kKeyGestureMoveDX[] = "dx";
const char ActivityLog::kKeyGestureMoveDY[] = "dy";
const char ActivityLog::kKeyGestureMoveOrdinalDX[] = "ordinalDx";
const char ActivityLog::kKeyGestureMoveOrdinalDY[] = "ordinalDy";
const char ActivityLog::kKeyGestureScrollDX[] = "dx";
const char ActivityLog::kKeyGestureScrollDY[] = "dy";
const char ActivityLog::kKeyGestureScrollOrdinalDX[] = "ordinalDx";
const char ActivityLog::kKeyGestureScrollOrdinalDY[] = "ordinalDy";
const char ActivityLog::kKeyGesturePinchDZ[] = "dz";
const char ActivityLog::kKeyGesturePinchOrdinalDZ[] = "ordinalDz";
const char ActivityLog::kKeyGesturePinchZoomState[] = "zoomState";
const char ActivityLog::kKeyGestureButtonsChangeDown[] = "down";
const char ActivityLog::kKeyGestureButtonsChangeUp[] = "up";
const char ActivityLog::kKeyGestureFlingVX[] = "vx";
const char ActivityLog::kKeyGestureFlingVY[] = "vy";
const char ActivityLog::kKeyGestureFlingOrdinalVX[] = "ordinalVx";
const char ActivityLog::kKeyGestureFlingOrdinalVY[] = "ordinalVy";
const char ActivityLog::kKeyGestureFlingState[] = "flingState";
const char ActivityLog::kKeyGestureSwipeDX[] = "dx";
const char ActivityLog::kKeyGestureSwipeDY[] = "dy";
const char ActivityLog::kKeyGestureSwipeOrdinalDX[] = "ordinalDx";
const char ActivityLog::kKeyGestureSwipeOrdinalDY[] = "ordinalDy";
const char ActivityLog::kKeyGestureFourFingerSwipeDX[] = "dx";
const char ActivityLog::kKeyGestureFourFingerSwipeDY[] = "dy";
const char ActivityLog::kKeyGestureFourFingerSwipeOrdinalDX[] = "ordinalDx";
const char ActivityLog::kKeyGestureFourFingerSwipeOrdinalDY[] = "ordinalDy";
const char ActivityLog::kKeyGestureMetricsType[] = "metricsType";
const char ActivityLog::kKeyGestureMetricsData1[] = "data1";
const char ActivityLog::kKeyGestureMetricsData2[] = "data2";
const char ActivityLog::kKeyPropChangeType[] = "propChangeType";
const char ActivityLog::kKeyPropChangeName[] = "name";
const char ActivityLog::kKeyPropChangeValue[] = "value";
const char ActivityLog::kValuePropChangeTypeBool[] = "bool";
const char ActivityLog::kValuePropChangeTypeDouble[] = "double";
const char ActivityLog::kValuePropChangeTypeInt[] = "int";
const char ActivityLog::kValuePropChangeTypeShort[] = "short";
const char ActivityLog::kKeyHardwarePropRoot[] = "hardwareProperties";
const char ActivityLog::kKeyHardwarePropLeft[] = "left";
const char ActivityLog::kKeyHardwarePropTop[] = "top";
const char ActivityLog::kKeyHardwarePropRight[] = "right";
const char ActivityLog::kKeyHardwarePropBottom[] = "bottom";
const char ActivityLog::kKeyHardwarePropXResolution[] = "xResolution";
const char ActivityLog::kKeyHardwarePropYResolution[] = "yResolution";
const char ActivityLog::kKeyHardwarePropXDpi[] = "xDpi";
const char ActivityLog::kKeyHardwarePropYDpi[] = "yDpi";
const char ActivityLog::kKeyHardwarePropOrientationMinimum[] =
    "orientationMinimum";
const char ActivityLog::kKeyHardwarePropOrientationMaximum[] =
    "orientationMaximum";
const char ActivityLog::kKeyHardwarePropMaxFingerCount[] = "maxFingerCount";
const char ActivityLog::kKeyHardwarePropMaxTouchCount[] = "maxTouchCount";
const char ActivityLog::kKeyHardwarePropSupportsT5R2[] = "supportsT5R2";
const char ActivityLog::kKeyHardwarePropSemiMt[] = "semiMt";
const char ActivityLog::kKeyHardwarePropIsButtonPad[] = "isButtonPad";
const char ActivityLog::kKeyHardwarePropHasWheel[] = "hasWheel";

const char ActivityLog::kKeyProperties[] = "properties";


}  // namespace gestures
