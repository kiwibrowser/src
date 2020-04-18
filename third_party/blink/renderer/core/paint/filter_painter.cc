// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/filter_painter.h"

#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/paint/filter_effect_builder.h"
#include "third_party/blink/renderer/core/paint/layer_clip_recorder.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/compositor_filter_operations.h"
#include "third_party/blink/renderer/platform/graphics/filters/filter_effect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/filter_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

sk_sp<PaintFilter> FilterPainter::GetImageFilter(PaintLayer& layer) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return nullptr;

  if (!layer.PaintsWithFilters())
    return nullptr;

  FilterEffect* last_effect = layer.LastFilterEffect();
  if (!last_effect)
    return nullptr;

  return PaintFilterBuilder::Build(last_effect, kInterpolationSpaceSRGB);
}

FilterPainter::FilterPainter(PaintLayer& layer,
                             GraphicsContext& context,
                             const LayoutPoint& offset_from_root,
                             const ClipRect& clip_rect,
                             PaintLayerPaintingInfo& painting_info,
                             PaintLayerFlags paint_flags)
    : filter_in_progress_(false),
      context_(context),
      layout_object_(layer.GetLayoutObject()) {
  sk_sp<PaintFilter> image_filter = GetImageFilter(layer);
  if (!image_filter)
    return;

  if (clip_rect.Rect() != painting_info.paint_dirty_rect ||
      clip_rect.HasRadius()) {
    // Apply clips outside the filter. See discussion about these clips
    // in PaintLayerPainter regarding "clipping in the presence of filters".
    clip_recorder_ = std::make_unique<LayerClipRecorder>(
        context, layer, DisplayItem::kClipLayerFilter, clip_rect,
        painting_info.root_layer, LayoutPoint(), paint_flags,
        layer.GetLayoutObject());
  }

  if (!context.GetPaintController().DisplayItemConstructionIsDisabled()) {
    CompositorFilterOperations compositor_filter_operations;
    layer.UpdateCompositorFilterOperationsForFilter(
        compositor_filter_operations);
    // FIXME: It's possible to have empty CompositorFilterOperations here even
    // though the PaintFilter produced above is non-null, since the
    // layer's FilterEffectBuilder can have a stale representation of
    // the layer's filter. See crbug.com/502026.
    if (compositor_filter_operations.IsEmpty())
      return;
    LayoutRect visual_bounds(
        layer.PhysicalBoundingBoxIncludingStackingChildren(offset_from_root));
    if (layer.EnclosingPaginationLayer()) {
      // Filters are set up before pagination, so we need to make the bounding
      // box visual on our own.
      visual_bounds.MoveBy(-offset_from_root);
      layer.ConvertFromFlowThreadToVisualBoundingBoxInAncestor(
          painting_info.root_layer, visual_bounds);
    }
    FloatPoint origin(offset_from_root);
    context.GetPaintController().CreateAndAppend<BeginFilterDisplayItem>(
        layout_object_, std::move(image_filter), FloatRect(visual_bounds),
        origin, std::move(compositor_filter_operations));
  }

  filter_in_progress_ = true;
}

FilterPainter::~FilterPainter() {
  if (!filter_in_progress_)
    return;

  context_.GetPaintController().EndItem<EndFilterDisplayItem>(layout_object_);
}

}  // namespace blink
