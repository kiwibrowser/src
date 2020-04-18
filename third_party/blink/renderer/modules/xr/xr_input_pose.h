// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_POSE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_POSE_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class XRInputPose final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  XRInputPose(std::unique_ptr<TransformationMatrix> pointer_matrix,
              std::unique_ptr<TransformationMatrix> grip_matrix,
              bool emulated_position = false);
  ~XRInputPose() override;

  DOMFloat32Array* pointerMatrix() const;
  DOMFloat32Array* gripMatrix() const;
  bool emulatedPosition() const { return emulated_position_; }

  void Trace(blink::Visitor*) override;

 private:
  const std::unique_ptr<TransformationMatrix> pointer_matrix_;
  const std::unique_ptr<TransformationMatrix> grip_matrix_;
  const bool emulated_position_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_POSE_H_
