// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_presentation_frame.h"

#include "third_party/blink/renderer/modules/xr/xr_coordinate_system.h"
#include "third_party/blink/renderer/modules/xr/xr_device_pose.h"
#include "third_party/blink/renderer/modules/xr/xr_input_pose.h"
#include "third_party/blink/renderer/modules/xr/xr_input_source.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"
#include "third_party/blink/renderer/modules/xr/xr_view.h"

namespace blink {

XRPresentationFrame::XRPresentationFrame(XRSession* session)
    : session_(session) {}

const HeapVector<Member<XRView>>& XRPresentationFrame::views() const {
  return session_->views();
}

XRDevicePose* XRPresentationFrame::getDevicePose(
    XRCoordinateSystem* coordinate_system) const {
  session_->LogGetPose();

  // If we don't have a valid base pose return null. Most common when tracking
  // is lost.
  if (!base_pose_matrix_ || !coordinate_system) {
    return nullptr;
  }

  // Must use a coordinate system created from the same session.
  if (coordinate_system->session() != session_) {
    return nullptr;
  }

  std::unique_ptr<TransformationMatrix> pose =
      coordinate_system->TransformBasePose(*base_pose_matrix_);

  if (!pose) {
    return nullptr;
  }

  return new XRDevicePose(session(), std::move(pose));
}

XRInputPose* XRPresentationFrame::getInputPose(
    XRInputSource* input_source,
    XRCoordinateSystem* coordinate_system) const {
  if (!input_source || !coordinate_system) {
    return nullptr;
  }

  // Must use an input source and coordinate system from the same session.
  if (input_source->session() != session_ ||
      coordinate_system->session() != session_) {
    return nullptr;
  }

  switch (input_source->pointer_origin_) {
    case XRInputSource::kOriginScreen: {
      // If the pointer origin is the screen we need the head's base pose and
      // the pointer transform matrix to continue. The pointer transform will
      // represent the point the canvas was clicked as an offset from the view.
      if (!base_pose_matrix_ || !input_source->pointer_transform_matrix_) {
        return nullptr;
      }

      // Multiply the head pose and pointer transform to get the final pointer.
      std::unique_ptr<TransformationMatrix> pointer_pose =
          coordinate_system->TransformBasePose(*base_pose_matrix_);
      pointer_pose->Multiply(*(input_source->pointer_transform_matrix_));

      return new XRInputPose(std::move(pointer_pose), nullptr);
    }
    case XRInputSource::kOriginHead: {
      // If the pointer origin is the users head, this is a gaze cursor and the
      // returned pointer is based on the device pose. If we don't have a valid
      // base pose (most common when tracking is lost) return null.
      if (!base_pose_matrix_) {
        return nullptr;
      }

      // Just return the head pose as the pointer pose.
      std::unique_ptr<TransformationMatrix> pointer_pose =
          coordinate_system->TransformBasePose(*base_pose_matrix_);

      return new XRInputPose(std::move(pointer_pose), nullptr,
                             input_source->emulatedPosition());
    }
    case XRInputSource::kOriginHand: {
      // If the input source doesn't have a base pose return null;
      if (!input_source->base_pose_matrix_) {
        return nullptr;
      }

      std::unique_ptr<TransformationMatrix> grip_pose =
          coordinate_system->TransformBaseInputPose(
              *(input_source->base_pose_matrix_), *base_pose_matrix_);

      if (!grip_pose) {
        return nullptr;
      }

      std::unique_ptr<TransformationMatrix> pointer_pose(
          TransformationMatrix::Create(*grip_pose));

      if (input_source->pointer_transform_matrix_) {
        pointer_pose->Multiply(*(input_source->pointer_transform_matrix_));
      }

      return new XRInputPose(std::move(pointer_pose), std::move(grip_pose),
                             input_source->emulatedPosition());
    }
  }

  return nullptr;
}

void XRPresentationFrame::SetBasePoseMatrix(
    const TransformationMatrix& base_pose_matrix) {
  base_pose_matrix_ = TransformationMatrix::Create(base_pose_matrix);
}

void XRPresentationFrame::Trace(blink::Visitor* visitor) {
  visitor->Trace(session_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
