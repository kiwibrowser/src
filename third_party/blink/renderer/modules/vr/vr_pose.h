// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_POSE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_POSE_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class VRPose final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRPose* Create() { return new VRPose(); }

  DOMFloat32Array* orientation() const { return orientation_; }
  DOMFloat32Array* position() const { return position_; }
  DOMFloat32Array* angularVelocity() const { return angular_velocity_; }
  DOMFloat32Array* linearVelocity() const { return linear_velocity_; }
  DOMFloat32Array* angularAcceleration() const { return angular_acceleration_; }
  DOMFloat32Array* linearAcceleration() const { return linear_acceleration_; }

  void SetPose(const device::mojom::blink::VRPosePtr&);

  void Trace(blink::Visitor*) override;

 private:
  VRPose();

  Member<DOMFloat32Array> orientation_;
  Member<DOMFloat32Array> position_;
  Member<DOMFloat32Array> angular_velocity_;
  Member<DOMFloat32Array> linear_velocity_;
  Member<DOMFloat32Array> angular_acceleration_;
  Member<DOMFloat32Array> linear_acceleration_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_POSE_H_
