// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_FRAME_OF_REFERENCE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_FRAME_OF_REFERENCE_H_

#include "third_party/blink/renderer/modules/xr/xr_coordinate_system.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"

namespace blink {

class XRStageBounds;

class XRFrameOfReference final : public XRCoordinateSystem {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum Type { kTypeHeadModel, kTypeEyeLevel, kTypeStage };

  XRFrameOfReference(XRSession*, Type);
  ~XRFrameOfReference() override;

  void UpdatePoseTransform(std::unique_ptr<TransformationMatrix>);
  void UpdateStageBounds(XRStageBounds*);
  void UseEmulatedHeight(double value);

  std::unique_ptr<TransformationMatrix> TransformBasePose(
      const TransformationMatrix& base_pose) override;
  std::unique_ptr<TransformationMatrix> TransformBaseInputPose(
      const TransformationMatrix& base_input_pose,
      const TransformationMatrix& base_pose) override;

  XRStageBounds* bounds() const { return bounds_; }
  double emulatedHeight() const { return emulated_height_; }

  Type type() const { return type_; }

  void Trace(blink::Visitor*) override;

 private:
  void UpdateStageTransform();

  Member<XRStageBounds> bounds_;
  double emulated_height_ = 0.0;
  Type type_;
  std::unique_ptr<TransformationMatrix> pose_transform_;
  unsigned int display_info_id_ = 0;
};

}  // namespace blink

#endif  // XRWebGLLayer_h
