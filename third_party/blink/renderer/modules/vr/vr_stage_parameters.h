// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_STAGE_PARAMETERS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_STAGE_PARAMETERS_H_

#include "device/vr/public/mojom/vr_service.mojom-blink.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class VRStageParameters final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  VRStageParameters();

  DOMFloat32Array* sittingToStandingTransform() const {
    return standing_transform_;
  }

  float sizeX() const { return size_x_; }
  float sizeZ() const { return size_z_; }

  void Update(const device::mojom::blink::VRStageParametersPtr&);

  void Trace(blink::Visitor*) override;

 private:
  Member<DOMFloat32Array> standing_transform_;
  float size_x_;
  float size_z_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_VR_VR_STAGE_PARAMETERS_H_
