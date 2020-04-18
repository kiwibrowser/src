// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_POSE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_POSE_H_

#include "device/gamepad/public/cpp/gamepad.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class GamepadPose final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GamepadPose* Create() { return new GamepadPose(); }

  bool hasOrientation() const { return has_orientation_; }
  bool hasPosition() const { return has_position_; }

  DOMFloat32Array* orientation() const { return orientation_; }
  DOMFloat32Array* position() const { return position_; }
  DOMFloat32Array* angularVelocity() const { return angular_velocity_; }
  DOMFloat32Array* linearVelocity() const { return linear_velocity_; }
  DOMFloat32Array* angularAcceleration() const { return angular_acceleration_; }
  DOMFloat32Array* linearAcceleration() const { return linear_acceleration_; }

  void SetPose(const device::GamepadPose& state);

  void Trace(blink::Visitor*) override;

 private:
  GamepadPose();

  bool has_orientation_;
  bool has_position_;

  Member<DOMFloat32Array> orientation_;
  Member<DOMFloat32Array> position_;
  Member<DOMFloat32Array> angular_velocity_;
  Member<DOMFloat32Array> linear_velocity_;
  Member<DOMFloat32Array> angular_acceleration_;
  Member<DOMFloat32Array> linear_acceleration_;
};

}  // namespace blink

#endif  // VRPose_h
