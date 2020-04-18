// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_FRAME_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_FRAME_DATA_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class VREyeParameters;
class VRPose;

class VRFrameData final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static VRFrameData* Create() { return new VRFrameData(); }

  VRFrameData();

  DOMFloat32Array* leftProjectionMatrix() const {
    return left_projection_matrix_;
  }
  DOMFloat32Array* leftViewMatrix() const { return left_view_matrix_; }
  DOMFloat32Array* rightProjectionMatrix() const {
    return right_projection_matrix_;
  }
  DOMFloat32Array* rightViewMatrix() const { return right_view_matrix_; }
  VRPose* pose() const { return pose_; }

  // Populate a the VRFrameData with a pose and the necessary eye parameters.
  // TODO(bajones): The full frame data should be provided by the VRService,
  // not computed here.
  bool Update(const device::mojom::blink::VRPosePtr&,
              VREyeParameters* left_eye,
              VREyeParameters* right_eye,
              float depth_near,
              float depth_far);

  void Trace(blink::Visitor*) override;

 private:
  Member<DOMFloat32Array> left_projection_matrix_;
  Member<DOMFloat32Array> left_view_matrix_;
  Member<DOMFloat32Array> right_projection_matrix_;
  Member<DOMFloat32Array> right_view_matrix_;
  Member<VRPose> pose_;
};

}  // namespace blink

#endif  // VRStageParameters_h
