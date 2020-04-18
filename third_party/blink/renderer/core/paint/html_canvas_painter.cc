// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/html_canvas_painter.h"

#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/layout/layout_html_canvas.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/foreign_layer_display_item.h"
#include "third_party/blink/renderer/platform/graphics/scoped_interpolation_quality.h"

namespace blink {

namespace {

InterpolationQuality InterpolationQualityForCanvas(const ComputedStyle& style) {
  if (style.ImageRendering() == EImageRendering::kWebkitOptimizeContrast)
    return kInterpolationLow;

  if (style.ImageRendering() == EImageRendering::kPixelated)
    return kInterpolationNone;

  return CanvasDefaultInterpolationQuality;
}

}  // namespace

void HTMLCanvasPainter::PaintReplaced(const PaintInfo& paint_info,
                                      const LayoutPoint& paint_offset) {
  GraphicsContext& context = paint_info.context;

  LayoutRect content_rect = layout_html_canvas_.ContentBoxRect();
  content_rect.MoveBy(paint_offset);
  LayoutRect paint_rect = layout_html_canvas_.ReplacedContentRect();
  paint_rect.MoveBy(paint_offset);

  HTMLCanvasElement* canvas =
      ToHTMLCanvasElement(layout_html_canvas_.GetNode());

  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
      canvas->RenderingContext() &&
      canvas->RenderingContext()->IsComposited()) {
    if (cc::Layer* layer = canvas->RenderingContext()->CcLayer()) {
      IntRect pixel_snapped_rect = PixelSnappedIntRect(content_rect);
      layer->SetBounds(static_cast<gfx::Size>(pixel_snapped_rect.Size()));
      layer->SetIsDrawable(true);
      RecordForeignLayer(
          context, layout_html_canvas_, DisplayItem::kForeignLayerCanvas, layer,
          pixel_snapped_rect.Location(), pixel_snapped_rect.Size());
      return;
    }
  }

  if (DrawingRecorder::UseCachedDrawingIfPossible(context, layout_html_canvas_,
                                                  paint_info.phase))
    return;

  DrawingRecorder recorder(context, layout_html_canvas_, paint_info.phase);

  bool clip = !content_rect.Contains(paint_rect);
  if (clip) {
    context.Save();
    // TODO(chrishtr): this should be pixel-snapped.
    context.Clip(FloatRect(content_rect));
  }

  {
    ScopedInterpolationQuality interpolation_quality_scope(
        context, InterpolationQualityForCanvas(layout_html_canvas_.StyleRef()));
    canvas->Paint(context, paint_rect);
  }

  if (clip)
    context.Restore();
}

}  // namespace blink
