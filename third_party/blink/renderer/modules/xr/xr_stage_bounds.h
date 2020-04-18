// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_H_

#include "third_party/blink/renderer/modules/xr/xr_stage_bounds_point.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class XRStageBounds final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  XRStageBounds() = default;

  HeapVector<Member<XRStageBoundsPoint>> geometry() const { return geometry_; }

  void Trace(blink::Visitor*) override;

 private:
  HeapVector<Member<XRStageBoundsPoint>> geometry_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_H_
