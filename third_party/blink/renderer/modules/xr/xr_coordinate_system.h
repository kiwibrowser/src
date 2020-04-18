// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_COORDINATE_SYSTEM_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_COORDINATE_SYSTEM_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class TransformationMatrix;
class XRSession;

class XRCoordinateSystem : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit XRCoordinateSystem(XRSession*);
  ~XRCoordinateSystem() override;

  DOMFloat32Array* getTransformTo(XRCoordinateSystem*) const;

  XRSession* session() { return session_; }

  virtual std::unique_ptr<TransformationMatrix> TransformBasePose(
      const TransformationMatrix& base_pose) = 0;
  virtual std::unique_ptr<TransformationMatrix> TransformBaseInputPose(
      const TransformationMatrix& base_input_pose,
      const TransformationMatrix& base_pose) = 0;

  void Trace(blink::Visitor*) override;

 private:
  const Member<XRSession> session_;
};

}  // namespace blink

#endif  // XRWebGLLayer_h
