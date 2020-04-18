// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/transform_3d_recorder.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_3d_display_item.h"

namespace blink {

Transform3DRecorder::Transform3DRecorder(GraphicsContext& context,
                                         const DisplayItemClient& client,
                                         DisplayItem::Type type,
                                         const TransformationMatrix& transform,
                                         const FloatPoint3D& transform_origin)
    : context_(context), client_(client), type_(type) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  DCHECK(DisplayItem::IsTransform3DType(type));
  skip_recording_for_identity_transform_ = transform.IsIdentity();

  if (skip_recording_for_identity_transform_)
    return;

  context_.GetPaintController().CreateAndAppend<BeginTransform3DDisplayItem>(
      client_, type_, transform, transform_origin);
}

Transform3DRecorder::~Transform3DRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  if (skip_recording_for_identity_transform_)
    return;

  context_.GetPaintController().EndItem<EndTransform3DDisplayItem>(
      client_, DisplayItem::Transform3DTypeToEndTransform3DType(type_));
}

}  // namespace blink
