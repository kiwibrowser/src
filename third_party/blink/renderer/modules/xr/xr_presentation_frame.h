// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_PRESENTATION_FRAME_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_PRESENTATION_FRAME_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class XRCoordinateSystem;
class XRDevicePose;
class XRInputPose;
class XRInputSource;
class XRSession;
class XRView;

class XRPresentationFrame final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit XRPresentationFrame(XRSession*);

  XRSession* session() const { return session_; }

  const HeapVector<Member<XRView>>& views() const;
  XRDevicePose* getDevicePose(XRCoordinateSystem*) const;
  XRInputPose* getInputPose(XRInputSource*, XRCoordinateSystem*) const;

  void SetBasePoseMatrix(const TransformationMatrix&);

  void Trace(blink::Visitor*) override;

 private:
  const Member<XRSession> session_;
  std::unique_ptr<TransformationMatrix> base_pose_matrix_;
};

}  // namespace blink

#endif  // XRWebGLLayer_h
