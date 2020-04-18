// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_LAYER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_LAYER_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/graphics/gpu/xr_webgl_drawing_buffer.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class XRSession;

enum XRLayerType { kXRWebGLLayerType };

class XRLayer : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  XRLayer(XRSession*, XRLayerType);

  XRSession* session() const { return session_; }
  XRLayerType layerType() const { return layer_type_; }

  virtual void OnFrameStart(const base::Optional<gpu::MailboxHolder>&);
  virtual void OnFrameEnd();
  virtual void OnResize();

  void Trace(blink::Visitor*) override;

 private:
  const Member<XRSession> session_;
  XRLayerType layer_type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_LAYER_H_
