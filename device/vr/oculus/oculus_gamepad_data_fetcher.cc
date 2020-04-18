// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/oculus/oculus_gamepad_data_fetcher.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "device/gamepad/public/cpp/gamepads.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/transform_util.h"

namespace device {

namespace {

float ApplyTriggerDeadzone(float value) {
  // Trigger value should be between 0 and 1.  We apply a deadzone for small
  // values so a loose controller still reports a value of 0 when not in use.
  float kTriggerDeadzone = 0.01f;

  return (value < kTriggerDeadzone) ? 0 : value;
}

void SetGamepadButton(Gamepad* pad,
                      const ovrInputState& input_state,
                      ovrButton button_id) {
  bool pressed = (input_state.Buttons & button_id) != 0;
  bool touched = (input_state.Touches & button_id) != 0;
  pad->buttons[pad->buttons_length].pressed = pressed;
  pad->buttons[pad->buttons_length].touched = touched;
  pad->buttons[pad->buttons_length].value = pressed ? 1.0f : 0.0f;
  pad->buttons_length++;
}

void SetGamepadTouchTrigger(Gamepad* pad,
                            const ovrInputState& input_state,
                            ovrTouch touch_id,
                            float value) {
  bool touched = (input_state.Touches & touch_id) != 0;
  value = ApplyTriggerDeadzone(value);
  pad->buttons[pad->buttons_length].pressed = value != 0;
  pad->buttons[pad->buttons_length].touched = touched;
  pad->buttons[pad->buttons_length].value = value;
  pad->buttons_length++;
}

void SetGamepadTrigger(Gamepad* pad, float value) {
  value = ApplyTriggerDeadzone(value);
  pad->buttons[pad->buttons_length].pressed = value != 0;
  pad->buttons[pad->buttons_length].touched = value != 0;
  pad->buttons[pad->buttons_length].value = value;
  pad->buttons_length++;
}

void SetGamepadTouch(Gamepad* pad,
                     const ovrInputState& input_state,
                     ovrTouch touch_id) {
  bool touched = (input_state.Touches & touch_id) != 0;
  pad->buttons[pad->buttons_length].pressed = false;
  pad->buttons[pad->buttons_length].touched = touched;
  pad->buttons[pad->buttons_length].value = 0.0f;
  pad->buttons_length++;
}

void SetTouchData(PadState* state,
                  const ovrPoseStatef& pose,
                  const ovrInputState& input_state,
                  ovrHandType hand,
                  unsigned int display_id) {
  if (!state)
    return;
  Gamepad& pad = state->data;
  if (!state->is_initialized) {
    state->is_initialized = true;
    pad.connected = true;
    pad.is_xr = true;
    pad.pose.not_null = true;
    pad.pose.has_orientation = true;
    pad.pose.has_position = true;
    pad.display_id = display_id;
    switch (hand) {
      case ovrHand_Left:
        swprintf(pad.id, Gamepad::kIdLengthCap, L"Oculus Touch (Left)");
        pad.hand = GamepadHand::kLeft;
        break;
      case ovrHand_Right:
        swprintf(pad.id, Gamepad::kIdLengthCap, L"Oculus Touch (Right)");
        pad.hand = GamepadHand::kRight;
        break;
      default:
        NOTREACHED();
        return;
    }
  }
  pad.timestamp = input_state.TimeInSeconds;
  pad.axes_length = 0;
  pad.buttons_length = 0;
  pad.axes[pad.axes_length++] = input_state.Thumbstick[hand].x;
  pad.axes[pad.axes_length++] = -input_state.Thumbstick[hand].y;

  // This gamepad layout is the defacto standard, but can be adjusted for WebXR.
  switch (hand) {
    case ovrHand_Left:
      SetGamepadButton(&pad, input_state, ovrButton_LThumb);
      break;
    case ovrHand_Right:
      SetGamepadButton(&pad, input_state, ovrButton_RThumb);
      break;
    default:
      NOTREACHED();
      break;
  }
  SetGamepadTouchTrigger(
      &pad, input_state,
      hand == ovrHand_Left ? ovrTouch_LIndexTrigger : ovrTouch_RIndexTrigger,
      input_state.IndexTrigger[hand]);
  SetGamepadTrigger(&pad, input_state.HandTrigger[hand]);
  switch (hand) {
    case ovrHand_Left:
      SetGamepadButton(&pad, input_state, ovrButton_X);
      SetGamepadButton(&pad, input_state, ovrButton_Y);
      SetGamepadTouch(&pad, input_state, ovrTouch_LThumbRest);
      break;
    case ovrHand_Right:
      SetGamepadButton(&pad, input_state, ovrButton_A);
      SetGamepadButton(&pad, input_state, ovrButton_B);
      SetGamepadTouch(&pad, input_state, ovrTouch_RThumbRest);
      break;
    default:
      NOTREACHED();
      break;
  }

  pad.pose.orientation.not_null = true;
  pad.pose.orientation.x = pose.ThePose.Orientation.x;
  pad.pose.orientation.y = pose.ThePose.Orientation.y;
  pad.pose.orientation.z = pose.ThePose.Orientation.z;
  pad.pose.orientation.w = pose.ThePose.Orientation.w;

  pad.pose.position.not_null = true;
  pad.pose.position.x = pose.ThePose.Position.x;
  pad.pose.position.y = pose.ThePose.Position.y;
  pad.pose.position.z = pose.ThePose.Position.z;

  pad.pose.angular_velocity.not_null = true;
  pad.pose.angular_velocity.x = pose.AngularVelocity.x;
  pad.pose.angular_velocity.y = pose.AngularVelocity.y;
  pad.pose.angular_velocity.z = pose.AngularVelocity.z;

  pad.pose.linear_velocity.not_null = true;
  pad.pose.linear_velocity.x = pose.LinearVelocity.x;
  pad.pose.linear_velocity.y = pose.LinearVelocity.y;
  pad.pose.linear_velocity.z = pose.LinearVelocity.z;

  pad.pose.angular_acceleration.not_null = true;
  pad.pose.angular_acceleration.x = pose.AngularAcceleration.x;
  pad.pose.angular_acceleration.y = pose.AngularAcceleration.y;
  pad.pose.angular_acceleration.z = pose.AngularAcceleration.z;

  pad.pose.linear_acceleration.not_null = true;
  pad.pose.linear_acceleration.x = pose.LinearAcceleration.x;
  pad.pose.linear_acceleration.y = pose.LinearAcceleration.y;
  pad.pose.linear_acceleration.z = pose.LinearAcceleration.z;
}

}  // namespace

OculusGamepadDataFetcher::Factory::Factory(unsigned int display_id,
                                           ovrSession session)
    : display_id_(display_id), session_(session) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OculusGamepadDataFetcher::Factory::~Factory() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

std::unique_ptr<GamepadDataFetcher>
OculusGamepadDataFetcher::Factory::CreateDataFetcher() {
  return std::make_unique<OculusGamepadDataFetcher>(display_id_, session_);
}

GamepadSource OculusGamepadDataFetcher::Factory::source() {
  return GAMEPAD_SOURCE_OCULUS;
}

OculusGamepadDataFetcher::OculusGamepadDataFetcher(unsigned int display_id,
                                                   ovrSession session)
    : display_id_(display_id), session_(session) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OculusGamepadDataFetcher::~OculusGamepadDataFetcher() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GamepadSource OculusGamepadDataFetcher::source() {
  return GAMEPAD_SOURCE_OCULUS;
}

void OculusGamepadDataFetcher::OnAddedToProvider() {}

void OculusGamepadDataFetcher::GetGamepadData(bool devices_changed_hint) {
  ovrInputState input_state;
  if ((OVR_SUCCESS(ovr_GetInputState(session_, ovrControllerType_Touch,
                                     &input_state)))) {
    ovrTrackingState tracking_state = ovr_GetTrackingState(session_, 0, false);
    SetTouchData(GetPadState(ovrControllerType_LTouch),
                 tracking_state.HandPoses[ovrHand_Left], input_state,
                 ovrHand_Left, display_id_);
    SetTouchData(GetPadState(ovrControllerType_RTouch),
                 tracking_state.HandPoses[ovrHand_Right], input_state,
                 ovrHand_Right, display_id_);
  }

  if ((OVR_SUCCESS(ovr_GetInputState(session_, ovrControllerType_Remote,
                                     &input_state)))) {
    PadState* state = GetPadState(ovrControllerType_Remote);
    if (state) {
      Gamepad& pad = state->data;
      if (!state->is_initialized) {
        state->is_initialized = true;
        swprintf(pad.id, Gamepad::kIdLengthCap, L"Oculus Remote");
        pad.connected = true;
        pad.is_xr = true;
        pad.display_id = display_id_;
      }
      pad.timestamp = input_state.TimeInSeconds;
      pad.axes_length = 0;
      pad.buttons_length = 0;
      SetGamepadButton(&pad, input_state, ovrButton_Enter);
      SetGamepadButton(&pad, input_state, ovrButton_Back);
      SetGamepadButton(&pad, input_state, ovrButton_Up);
      SetGamepadButton(&pad, input_state, ovrButton_Down);
      SetGamepadButton(&pad, input_state, ovrButton_Left);
      SetGamepadButton(&pad, input_state, ovrButton_Right);
    }
  }
}

void OculusGamepadDataFetcher::PauseHint(bool paused) {}

}  // namespace device
