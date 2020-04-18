// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/openvr/openvr_gamepad_data_fetcher.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "device/gamepad/public/cpp/gamepads.h"
#include "third_party/openvr/src/headers/openvr.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/transform_util.h"

namespace device {

namespace {

void SetGamepadButton(Gamepad* pad,
                      const vr::VRControllerState_t& controller_state,
                      uint64_t supported_buttons,
                      vr::EVRButtonId button_id) {
  uint64_t button_mask = vr::ButtonMaskFromId(button_id);
  if ((supported_buttons & button_mask) != 0) {
    bool button_pressed = (controller_state.ulButtonPressed & button_mask) != 0;
    bool button_touched = (controller_state.ulButtonTouched & button_mask) != 0;
    pad->buttons[pad->buttons_length].touched = button_touched;
    pad->buttons[pad->buttons_length].pressed = button_pressed;
    pad->buttons[pad->buttons_length].value = button_pressed ? 1.0 : 0.0;
    pad->buttons_length++;
  }
}

}  // namespace

OpenVRGamepadDataFetcher::Factory::Factory(unsigned int display_id,
                                           vr::IVRSystem* vr)
    : display_id_(display_id), vr_system_(vr) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OpenVRGamepadDataFetcher::Factory::~Factory() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

std::unique_ptr<GamepadDataFetcher>
OpenVRGamepadDataFetcher::Factory::CreateDataFetcher() {
  return std::make_unique<OpenVRGamepadDataFetcher>(display_id_, vr_system_);
}

GamepadSource OpenVRGamepadDataFetcher::Factory::source() {
  return GAMEPAD_SOURCE_OPENVR;
}

OpenVRGamepadDataFetcher::OpenVRGamepadDataFetcher(unsigned int display_id,
                                                   vr::IVRSystem* vr)
    : display_id_(display_id), vr_system_(vr) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OpenVRGamepadDataFetcher::~OpenVRGamepadDataFetcher() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GamepadSource OpenVRGamepadDataFetcher::source() {
  return GAMEPAD_SOURCE_OPENVR;
}

void OpenVRGamepadDataFetcher::OnAddedToProvider() {}

void OpenVRGamepadDataFetcher::GetGamepadData(bool devices_changed_hint) {
  if (!vr_system_)
    return;

  vr::TrackedDevicePose_t tracked_devices_poses[vr::k_unMaxTrackedDeviceCount];
  vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseSeated, 0.0f,
                                              tracked_devices_poses,
                                              vr::k_unMaxTrackedDeviceCount);

  for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
    if (vr_system_->GetTrackedDeviceClass(i) !=
        vr::TrackedDeviceClass_Controller)
      continue;

    PadState* state = GetPadState(i);
    if (!state)
      continue;

    Gamepad& pad = state->data;

    vr::VRControllerState_t controller_state;
    if (vr_system_->GetControllerState(i, &controller_state,
                                       sizeof(controller_state))) {
      pad.timestamp = controller_state.unPacketNum;
      pad.connected = true;
      pad.is_xr = true;

      pad.pose.not_null = true;

      pad.pose.has_orientation = true;
      pad.pose.has_position = true;

      // The defacto standard says we set id to "OpenVR Gamepad".  WebXR input
      // will provide a better solution.
      swprintf(pad.id, Gamepad::kIdLengthCap, L"OpenVR Gamepad");
      swprintf(pad.mapping, Gamepad::kMappingLengthCap, L"");

      pad.display_id = display_id_;

      vr::ETrackedControllerRole hand =
          vr_system_->GetControllerRoleForTrackedDeviceIndex(i);

      switch (hand) {
        case vr::TrackedControllerRole_Invalid:
          pad.hand = GamepadHand::kNone;
          break;
        case vr::TrackedControllerRole_LeftHand:
          pad.hand = GamepadHand::kLeft;
          break;
        case vr::TrackedControllerRole_RightHand:
          pad.hand = GamepadHand::kRight;
          break;
      }

      uint64_t supported_buttons = vr_system_->GetUint64TrackedDeviceProperty(
          i, vr::Prop_SupportedButtons_Uint64);

      pad.buttons_length = 0;
      pad.axes_length = 0;

      for (unsigned int j = 0; j < vr::k_unControllerStateAxisCount; ++j) {
        int32_t axis_type = vr_system_->GetInt32TrackedDeviceProperty(
            i, static_cast<vr::TrackedDeviceProperty>(vr::Prop_Axis0Type_Int32 +
                                                      j));
        switch (axis_type) {
          case vr::k_eControllerAxis_Joystick:
          case vr::k_eControllerAxis_TrackPad:
            pad.axes[pad.axes_length++] = controller_state.rAxis[j].x;
            pad.axes[pad.axes_length++] = controller_state.rAxis[j].y;

            SetGamepadButton(
                &pad, controller_state, supported_buttons,
                static_cast<vr::EVRButtonId>(vr::k_EButton_Axis0 + j));

            break;
          case vr::k_eControllerAxis_Trigger:
            pad.buttons[pad.buttons_length].value = controller_state.rAxis[j].x;

            uint64_t button_mask = vr::ButtonMaskFromId(
                static_cast<vr::EVRButtonId>(vr::k_EButton_Axis0 + j));
            if ((supported_buttons & button_mask) != 0) {
              pad.buttons[pad.buttons_length].pressed =
                  (controller_state.ulButtonPressed & button_mask) != 0;
            }

            pad.buttons_length++;
            break;
        }
      }

      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_A);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_Grip);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_ApplicationMenu);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_DPad_Left);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_DPad_Up);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_DPad_Right);
      SetGamepadButton(&pad, controller_state, supported_buttons,
                       vr::k_EButton_DPad_Down);
    }

    const vr::TrackedDevicePose_t& pose = tracked_devices_poses[i];
    if (pose.bPoseIsValid) {
      const vr::HmdMatrix34_t& mat = pose.mDeviceToAbsoluteTracking;
      gfx::Transform transform(
          mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3], mat.m[1][0],
          mat.m[1][1], mat.m[1][2], mat.m[1][3], mat.m[2][0], mat.m[2][1],
          mat.m[2][2], mat.m[2][3], 0, 0, 0, 1);

      gfx::DecomposedTransform decomposed_transform;
      gfx::DecomposeTransform(&decomposed_transform, transform);

      pad.pose.orientation.not_null = true;
      pad.pose.orientation.x = decomposed_transform.quaternion.x();
      pad.pose.orientation.y = decomposed_transform.quaternion.y();
      pad.pose.orientation.z = decomposed_transform.quaternion.z();
      pad.pose.orientation.w = decomposed_transform.quaternion.w();

      pad.pose.position.not_null = true;
      pad.pose.position.x = decomposed_transform.translate[0];
      pad.pose.position.y = decomposed_transform.translate[1];
      pad.pose.position.z = decomposed_transform.translate[2];

      pad.pose.angular_velocity.not_null = true;
      pad.pose.angular_velocity.x = pose.vAngularVelocity.v[0];
      pad.pose.angular_velocity.y = pose.vAngularVelocity.v[1];
      pad.pose.angular_velocity.z = pose.vAngularVelocity.v[2];

      pad.pose.linear_velocity.not_null = true;
      pad.pose.linear_velocity.x = pose.vVelocity.v[0];
      pad.pose.linear_velocity.y = pose.vVelocity.v[1];
      pad.pose.linear_velocity.z = pose.vVelocity.v[2];
    } else {
      pad.pose.orientation.not_null = false;
      pad.pose.position.not_null = false;
      pad.pose.angular_velocity.not_null = false;
      pad.pose.linear_velocity.not_null = false;
    }
  }
}

void OpenVRGamepadDataFetcher::PauseHint(bool paused) {}

}  // namespace device
