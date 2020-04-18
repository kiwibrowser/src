// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/android/gvr/gvr_gamepad_data_fetcher.h"

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "device/gamepad/public/cpp/gamepads.h"
#include "device/vr/android/gvr/gvr_gamepad_data_provider.h"
#include "ui/gfx/geometry/quaternion.h"

namespace device {

namespace {

void CopyToUString(UChar* dest, size_t dest_length, base::string16 src) {
  static_assert(sizeof(base::string16::value_type) == sizeof(UChar),
                "Mismatched string16/WebUChar size.");

  const size_t str_to_copy = std::min(src.size(), dest_length - 1);
  memcpy(dest, src.data(), str_to_copy * sizeof(base::string16::value_type));
  dest[str_to_copy] = 0;
}

}  // namespace

GvrGamepadDataFetcher::Factory::Factory(GvrGamepadDataProvider* data_provider,
                                        unsigned int display_id)
    : data_provider_(data_provider), display_id_(display_id) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GvrGamepadDataFetcher::Factory::~Factory() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

std::unique_ptr<GamepadDataFetcher>
GvrGamepadDataFetcher::Factory::CreateDataFetcher() {
  return std::make_unique<GvrGamepadDataFetcher>(data_provider_, display_id_);
}

GamepadSource GvrGamepadDataFetcher::Factory::source() {
  return GAMEPAD_SOURCE_GVR;
}

GvrGamepadDataFetcher::GvrGamepadDataFetcher(
    GvrGamepadDataProvider* data_provider,
    unsigned int display_id)
    : display_id_(display_id) {
  // Called on UI thread.
  DVLOG(1) << __FUNCTION__ << "=" << this;
  data_provider->RegisterGvrGamepadDataFetcher(this);
}

GvrGamepadDataFetcher::~GvrGamepadDataFetcher() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GamepadSource GvrGamepadDataFetcher::source() {
  return GAMEPAD_SOURCE_GVR;
}

void GvrGamepadDataFetcher::OnAddedToProvider() {
  PauseHint(false);
}

void GvrGamepadDataFetcher::SetGamepadData(GvrGamepadData data) {
  // Called from UI thread.
  gamepad_data_ = data;
}

void GvrGamepadDataFetcher::GetGamepadData(bool devices_changed_hint) {
  // Called from gamepad polling thread.

  PadState* state = GetPadState(0);
  if (!state)
    return;

  // Take a snapshot of the asynchronously updated gamepad data.
  // TODO(bajones): ensure consistency?
  GvrGamepadData provided_data = gamepad_data_;

  Gamepad& pad = state->data;
  if (!state->is_initialized) {
    state->is_initialized = true;
    // This is the first time we've seen this device, so do some one-time
    // initialization
    CopyToUString(pad.id, Gamepad::kIdLengthCap,
                  base::UTF8ToUTF16("Daydream Controller"));
    CopyToUString(pad.mapping, Gamepad::kMappingLengthCap,
                  base::UTF8ToUTF16(""));
    pad.buttons_length = 1;
    pad.axes_length = 2;

    pad.display_id = display_id_;

    pad.is_xr = true;

    pad.hand =
        provided_data.right_handed ? GamepadHand::kRight : GamepadHand::kLeft;
  }

  pad.connected = provided_data.connected;
  pad.timestamp = provided_data.timestamp;

  if (provided_data.is_touching) {
    gfx::Vector2dF touch_position = provided_data.touch_pos;
    pad.axes[0] = (touch_position.x() * 2.0f) - 1.0f;
    pad.axes[1] = (touch_position.y() * 2.0f) - 1.0f;
  } else {
    pad.axes[0] = 0.0f;
    pad.axes[1] = 0.0f;
  }

  pad.buttons[0].touched = provided_data.is_touching;
  pad.buttons[0].pressed = provided_data.controller_button_pressed;
  pad.buttons[0].value = pad.buttons[0].pressed ? 1.0f : 0.0f;

  pad.pose.not_null = true;
  pad.pose.has_orientation = true;
  pad.pose.has_position = false;

  gfx::Quaternion orientation = provided_data.orientation;
  pad.pose.orientation.not_null = true;
  pad.pose.orientation.x = orientation.x();
  pad.pose.orientation.y = orientation.y();
  pad.pose.orientation.z = orientation.z();
  pad.pose.orientation.w = orientation.w();

  gfx::Vector3dF accel = provided_data.accel;
  pad.pose.linear_acceleration.not_null = true;
  pad.pose.linear_acceleration.x = accel.x();
  pad.pose.linear_acceleration.y = accel.y();
  pad.pose.linear_acceleration.z = accel.z();

  gfx::Vector3dF gyro = provided_data.gyro;
  pad.pose.angular_velocity.not_null = true;
  pad.pose.angular_velocity.x = gyro.x();
  pad.pose.angular_velocity.y = gyro.y();
  pad.pose.angular_velocity.z = gyro.z();
}

void GvrGamepadDataFetcher::PauseHint(bool paused) {}

}  // namespace device
