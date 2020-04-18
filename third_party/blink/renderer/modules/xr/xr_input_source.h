// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class XRSession;

class XRInputSource : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum Handedness { kHandNone = 0, kHandLeft = 1, kHandRight = 2 };
  enum PointerOrigin { kOriginHead = 1, kOriginHand = 2, kOriginScreen = 3 };

  XRInputSource(XRSession*, uint32_t source_id);
  ~XRInputSource() override = default;

  XRSession* session() const { return session_; }

  const String& handedness() const { return handedness_string_; }
  const String& pointerOrigin() const { return pointer_origin_string_; }
  bool emulatedPosition() const { return emulated_position_; }

  uint32_t source_id() const { return source_id_; }

  void SetPointerOrigin(PointerOrigin);
  void SetHandedness(Handedness);
  void SetEmulatedPosition(bool emulated_position);
  void SetBasePoseMatrix(std::unique_ptr<TransformationMatrix>);
  void SetPointerTransformMatrix(std::unique_ptr<TransformationMatrix>);

  void Trace(blink::Visitor*) override;

  int16_t active_frame_id = -1;
  bool primary_input_pressed = false;
  bool selection_cancelled = false;

 private:
  friend class XRPresentationFrame;

  const Member<XRSession> session_;
  const uint32_t source_id_;

  Handedness handedness_;
  String handedness_string_;

  PointerOrigin pointer_origin_;
  String pointer_origin_string_;

  bool emulated_position_ = false;

  std::unique_ptr<TransformationMatrix> base_pose_matrix_;

  // This is the transform to apply to the base_pose_matrix_ to get the pointer
  // matrix. In most cases it should be static.
  std::unique_ptr<TransformationMatrix> pointer_transform_matrix_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_INPUT_SOURCE_H_
