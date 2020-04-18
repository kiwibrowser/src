// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/gamepad/gamepad_pose.h"

namespace blink {

namespace {

DOMFloat32Array* VecToFloat32Array(const device::GamepadVector& vec) {
  if (vec.not_null) {
    DOMFloat32Array* out = DOMFloat32Array::Create(3);
    out->Data()[0] = vec.x;
    out->Data()[1] = vec.y;
    out->Data()[2] = vec.z;
    return out;
  }
  return nullptr;
}

DOMFloat32Array* QuatToFloat32Array(const device::GamepadQuaternion& quat) {
  if (quat.not_null) {
    DOMFloat32Array* out = DOMFloat32Array::Create(4);
    out->Data()[0] = quat.x;
    out->Data()[1] = quat.y;
    out->Data()[2] = quat.z;
    out->Data()[3] = quat.w;
    return out;
  }
  return nullptr;
}

}  // namespace

GamepadPose::GamepadPose() = default;

void GamepadPose::SetPose(const device::GamepadPose& state) {
  if (state.not_null) {
    has_orientation_ = state.has_orientation;
    has_position_ = state.has_position;

    orientation_ = QuatToFloat32Array(state.orientation);
    position_ = VecToFloat32Array(state.position);
    angular_velocity_ = VecToFloat32Array(state.angular_velocity);
    linear_velocity_ = VecToFloat32Array(state.linear_velocity);
    angular_acceleration_ = VecToFloat32Array(state.angular_acceleration);
    linear_acceleration_ = VecToFloat32Array(state.linear_acceleration);
  }
}

void GamepadPose::Trace(blink::Visitor* visitor) {
  visitor->Trace(orientation_);
  visitor->Trace(position_);
  visitor->Trace(angular_velocity_);
  visitor->Trace(linear_velocity_);
  visitor->Trace(angular_acceleration_);
  visitor->Trace(linear_acceleration_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
