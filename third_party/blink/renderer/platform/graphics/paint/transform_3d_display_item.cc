// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/transform_3d_display_item.h"

#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"

namespace blink {

void BeginTransform3DDisplayItem::Replay(GraphicsContext& context) const {
  TransformationMatrix transform(transform_);
  transform.ApplyTransformOrigin(transform_origin_);
  context.Save();
  context.ConcatCTM(transform.ToAffineTransform());
}

void BeginTransform3DDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  // TODO(jbroman): The compositor will need the transform origin separately.
  TransformationMatrix transform(transform_);
  transform.ApplyTransformOrigin(transform_origin_);

  list.StartPaint();
  list.push<cc::SaveOp>();
  if (!transform.IsIdentity()) {
    list.push<cc::ConcatOp>(
        static_cast<SkMatrix>(TransformationMatrix::ToSkMatrix44(transform)));
  }
  list.EndPaintOfPairedBegin();
}

#if DCHECK_IS_ON()
void BeginTransform3DDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  PairedBeginDisplayItem::PropertiesAsJSON(json);
  json.SetString("transform", transform_.ToString());
  json.SetString("origin", transform_origin_.ToString());
}
#endif

void EndTransform3DDisplayItem::Replay(GraphicsContext& context) const {
  context.Restore();
}

void EndTransform3DDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::RestoreOp>();
  list.EndPaintOfPairedEnd();
}

}  // namespace blink
