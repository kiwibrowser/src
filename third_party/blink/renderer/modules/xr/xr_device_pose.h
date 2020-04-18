// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_DEVICE_POSE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_DEVICE_POSE_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"

namespace blink {

class XRSession;
class XRView;

class XRDevicePose final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  XRDevicePose(XRSession*, std::unique_ptr<TransformationMatrix>);

  DOMFloat32Array* poseModelMatrix() const;
  DOMFloat32Array* getViewMatrix(XRView*);

  void Trace(blink::Visitor*) override;

 private:
  const Member<XRSession> session_;
  std::unique_ptr<TransformationMatrix> pose_model_matrix_;
};

}  // namespace blink

#endif  // XRWebGLLayer_h
