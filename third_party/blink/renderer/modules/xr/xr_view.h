// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_VIEW_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_VIEW_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/geometry/float_point_3d.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class XRSession;

class XRView final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum Eye { kEyeLeft = 0, kEyeRight = 1 };

  XRView(XRSession*, Eye);

  const String& eye() const { return eye_string_; }
  Eye EyeValue() const { return eye_; }

  XRSession* session() const;
  DOMFloat32Array* projectionMatrix() const { return projection_matrix_; }

  void UpdateProjectionMatrixFromRawValues(
      const WTF::Vector<float>& projection_matrix,
      float near_depth,
      float far_depth);

  void UpdateProjectionMatrixFromFoV(float up_rad,
                                     float down_rad,
                                     float left_rad,
                                     float right_rad,
                                     float near_depth,
                                     float far_depth);

  void UpdateProjectionMatrixFromAspect(float fovy,
                                        float aspect,
                                        float near_depth,
                                        float far_depth);

  std::unique_ptr<TransformationMatrix> UnprojectPointer(double x,
                                                         double y,
                                                         double canvas_width,
                                                         double canvas_height);

  // TODO(bajones): Should eventually represent this as a full transform.
  const FloatPoint3D& offset() const { return offset_; }
  void UpdateOffset(float x, float y, float z);

  void Trace(blink::Visitor*) override;

 private:
  const Eye eye_;
  String eye_string_;
  Member<XRSession> session_;
  Member<DOMFloat32Array> projection_matrix_;
  FloatPoint3D offset_;
  std::unique_ptr<TransformationMatrix> inv_projection_;
  bool inv_projection_dirty_ = true;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_VIEW_H_
