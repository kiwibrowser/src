// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_input_source.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"

namespace blink {

XRInputSource::XRInputSource(XRSession* session, uint32_t source_id)
    : session_(session), source_id_(source_id) {
  SetPointerOrigin(kOriginHead);
  SetHandedness(kHandNone);
}

void XRInputSource::SetPointerOrigin(PointerOrigin pointer_origin) {
  if (pointer_origin_ == pointer_origin)
    return;

  pointer_origin_ = pointer_origin;

  switch (pointer_origin_) {
    case kOriginHead:
      pointer_origin_string_ = "head";
      break;
    case kOriginHand:
      pointer_origin_string_ = "hand";
      break;
    case kOriginScreen:
      pointer_origin_string_ = "screen";
      break;
    default:
      NOTREACHED() << "Unknown pointer origin: " << pointer_origin_;
  }
}

void XRInputSource::SetHandedness(Handedness handedness) {
  if (handedness_ == handedness)
    return;

  handedness_ = handedness;

  switch (handedness_) {
    case kHandNone:
      handedness_string_ = "";
      break;
    case kHandLeft:
      handedness_string_ = "left";
      break;
    case kHandRight:
      handedness_string_ = "right";
      break;
    default:
      NOTREACHED() << "Unknown handedness: " << handedness_;
  }
}

void XRInputSource::SetEmulatedPosition(bool emulated_position) {
  emulated_position_ = emulated_position;
}

void XRInputSource::SetBasePoseMatrix(
    std::unique_ptr<TransformationMatrix> base_pose_matrix) {
  base_pose_matrix_ = std::move(base_pose_matrix);
}

void XRInputSource::SetPointerTransformMatrix(
    std::unique_ptr<TransformationMatrix> pointer_transform_matrix) {
  pointer_transform_matrix_ = std::move(pointer_transform_matrix);
}

void XRInputSource::Trace(blink::Visitor* visitor) {
  visitor->Trace(session_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
