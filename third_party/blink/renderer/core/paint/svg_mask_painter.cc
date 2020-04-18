// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/svg_mask_painter.h"

#include "third_party/blink/renderer/core/layout/svg/layout_svg_resource_masker.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/paint/compositing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/compositing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"

namespace blink {

bool SVGMaskPainter::PrepareEffect(const LayoutObject& object,
                                   GraphicsContext& context) {
  DCHECK(mask_.Style());
  SECURITY_DCHECK(!mask_.NeedsLayout());

  mask_.ClearInvalidationMask();

  FloatRect visual_rect = object.VisualRectInLocalSVGCoordinates();
  if (visual_rect.IsEmpty() || !mask_.GetElement()->HasChildren())
    return false;

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    context.GetPaintController().CreateAndAppend<BeginCompositingDisplayItem>(
        object, SkBlendMode::kSrcOver, 1, &visual_rect);
  }
  return true;
}

void SVGMaskPainter::FinishEffect(const LayoutObject& object,
                                  GraphicsContext& context) {
  DCHECK(mask_.Style());
  SECURITY_DCHECK(!mask_.NeedsLayout());

  FloatRect visual_rect = object.VisualRectInLocalSVGCoordinates();
  {
    ColorFilter mask_layer_filter =
        mask_.Style()->SvgStyle().MaskType() == MT_LUMINANCE
            ? kColorFilterLuminanceToAlpha
            : kColorFilterNone;
    CompositingRecorder mask_compositing(context, object, SkBlendMode::kDstIn,
                                         1, &visual_rect, mask_layer_filter);
    base::Optional<ScopedPaintChunkProperties> scoped_paint_chunk_properties;
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      const auto* properties = object.FirstFragment().PaintProperties();
      // TODO(crbug.com/814815): This condition should be a DCHECK, but for now
      // we may paint the object for filters during PrePaint before the
      // properties are ready.
      if (properties && properties->Mask()) {
        scoped_paint_chunk_properties.emplace(context.GetPaintController(),
                                              properties->Mask(), object,
                                              DisplayItem::kSVGMask);
      }
    }

    DrawMaskForLayoutObject(context, object, object.ObjectBoundingBox());
  }

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    context.GetPaintController().EndItem<EndCompositingDisplayItem>(object);
}

void SVGMaskPainter::DrawMaskForLayoutObject(
    GraphicsContext& context,
    const LayoutObject& layout_object,
    const FloatRect& target_bounding_box) {
  AffineTransform content_transformation;
  sk_sp<const PaintRecord> record = mask_.CreatePaintRecord(
      content_transformation, target_bounding_box, context);

  if (DrawingRecorder::UseCachedDrawingIfPossible(context, layout_object,
                                                  DisplayItem::kSVGMask))
    return;

  DrawingRecorder recorder(context, layout_object, DisplayItem::kSVGMask);
  context.Save();
  context.ConcatCTM(content_transformation);
  context.DrawRecord(std::move(record));
  context.Restore();
}

}  // namespace blink
