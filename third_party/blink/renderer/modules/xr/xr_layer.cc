// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_layer.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"

#include "device/vr/public/mojom/vr_service.mojom-blink.h"

namespace blink {

XRLayer::XRLayer(XRSession* session, XRLayerType layer_type)
    : session_(session), layer_type_(layer_type) {}

void XRLayer::OnFrameStart(const base::Optional<gpu::MailboxHolder>&) {}
void XRLayer::OnFrameEnd() {}
void XRLayer::OnResize() {}

void XRLayer::Trace(blink::Visitor* visitor) {
  visitor->Trace(session_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
