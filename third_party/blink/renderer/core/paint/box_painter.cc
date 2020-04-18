// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_painter.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/background_bleed_avoidance.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_table.h"
#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_foreign_object.h"
#include "third_party/blink/renderer/core/paint/adjust_paint_offset_scope.h"
#include "third_party/blink/renderer/core/paint/background_image_geometry.h"
#include "third_party/blink/renderer/core/paint/box_decoration_data.h"
#include "third_party/blink/renderer/core/paint/box_model_object_painter.h"
#include "third_party/blink/renderer/core/paint/box_painter_base.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/nine_piece_image_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/scroll_recorder.h"
#include "third_party/blink/renderer/core/paint/svg_foreign_object_painter.h"
#include "third_party/blink/renderer/core/paint/theme_painter.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_cache_skipper.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/length_functions.h"

namespace blink {

void BoxPainter::Paint(const PaintInfo& paint_info,
                       const LayoutPoint& paint_offset) {
  // Default implementation. Just pass paint through to the children.
  AdjustPaintOffsetScope adjustment(layout_box_, paint_info, paint_offset);
  PaintChildren(adjustment.GetPaintInfo(), adjustment.AdjustedPaintOffset());
}

void BoxPainter::PaintChildren(const PaintInfo& paint_info,
                               const LayoutPoint& paint_offset) {
  PaintInfo child_info(paint_info);
  for (LayoutObject* child = layout_box_.SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        child->IsSVGForeignObject()) {
      SVGForeignObjectPainter(ToLayoutSVGForeignObject(*child))
          .PaintLayer(paint_info);
    } else {
      child->Paint(child_info, paint_offset);
    }
  }
}

void BoxPainter::PaintBoxDecorationBackground(const PaintInfo& paint_info,
                                              const LayoutPoint& paint_offset) {
  LayoutRect paint_rect;
  base::Optional<ScrollRecorder> scroll_recorder;
  base::Optional<ScopedPaintChunkProperties> scoped_scroll_property;
  if (BoxModelObjectPainter::
          IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
              &layout_box_, paint_info)) {
    // For the case where we are painting the background into the scrolling
    // contents layer of a composited scroller we need to include the entire
    // overflow rect.
    paint_rect = layout_box_.PhysicalLayoutOverflowRect();

    scroll_recorder.emplace(paint_info.context, layout_box_, paint_info.phase,
                            layout_box_.ScrolledContentOffset());
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      if (const auto* fragment = paint_info.FragmentToPaint(layout_box_)) {
        scoped_scroll_property.emplace(
            paint_info.context.GetPaintController(),
            fragment->ContentsProperties(), layout_box_,
            DisplayItem::PaintPhaseToScrollType(paint_info.phase));
      }
    }

    // The background painting code assumes that the borders are part of the
    // paintRect so we expand the paintRect by the border size when painting the
    // background into the scrolling contents layer.
    paint_rect.Expand(layout_box_.BorderBoxOutsets());
  } else {
    paint_rect = layout_box_.BorderBoxRect();
  }

  paint_rect.MoveBy(paint_offset);
  PaintBoxDecorationBackgroundWithRect(paint_info, paint_offset, paint_rect);
}

LayoutRect BoxPainter::BoundsForDrawingRecorder(
    const PaintInfo& paint_info,
    const LayoutPoint& adjusted_paint_offset) {
  LayoutRect bounds =
      BoxModelObjectPainter::
              IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
                  &layout_box_, paint_info)
          ? layout_box_.LayoutOverflowRect()
          : layout_box_.SelfVisualOverflowRect();
  bounds.MoveBy(adjusted_paint_offset);
  return bounds;
}

void BoxPainter::PaintBoxDecorationBackgroundWithRect(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutRect& paint_rect) {
  bool painting_overflow_contents = BoxModelObjectPainter::
      IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          &layout_box_, paint_info);
  const ComputedStyle& style = layout_box_.StyleRef();

  base::Optional<DisplayItemCacheSkipper> cache_skipper;
  // Disable cache in under-invalidation checking mode for MediaSliderPart
  // because we always paint using the latest data (buffered ranges, current
  // time and duration) which may be different from the cached data, and for
  // delayed-invalidation object because it may change before it's actually
  // invalidated. Note that we still report harmless under-invalidation of
  // non-delayed-invalidation animated background, which should be ignored.
  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      (style.Appearance() == kMediaSliderPart ||
       layout_box_.FullPaintInvalidationReason() ==
           PaintInvalidationReason::kDelayedFull)) {
    cache_skipper.emplace(paint_info.context);
  }

  const DisplayItemClient& display_item_client =
      painting_overflow_contents ? static_cast<const DisplayItemClient&>(
                                       *layout_box_.Layer()
                                            ->GetCompositedLayerMapping()
                                            ->ScrollingContentsLayer())
                                 : layout_box_;
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, display_item_client,
          DisplayItem::kBoxDecorationBackground))
    return;

  DrawingRecorder recorder(paint_info.context, display_item_client,
                           DisplayItem::kBoxDecorationBackground);
  BoxDecorationData box_decoration_data(layout_box_);
  GraphicsContextStateSaver state_saver(paint_info.context, false);

  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
      LayoutRect(EnclosingIntRect(paint_rect)) == paint_rect &&
      // TODO(wkorman): Rename BoundsForDrawingRecorder as it's used
      // here for another purpose.
      layout_box_.BackgroundIsKnownToBeOpaqueInRect(
          BoundsForDrawingRecorder(paint_info, LayoutPoint())))
    recorder.SetKnownToBeOpaque();

  bool needs_end_layer = false;
  if (!painting_overflow_contents) {
    // FIXME: Should eventually give the theme control over whether the box
    // shadow should paint, since controls could have custom shadows of their
    // own.
    BoxPainterBase::PaintNormalBoxShadow(paint_info, paint_rect, style);

    if (BleedAvoidanceIsClipping(box_decoration_data.bleed_avoidance)) {
      state_saver.Save();
      FloatRoundedRect border = style.GetRoundedBorderFor(paint_rect);
      paint_info.context.ClipRoundedRect(border);

      if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer) {
        paint_info.context.BeginLayer();
        needs_end_layer = true;
      }
    }
  }

  // If we have a native theme appearance, paint that before painting our
  // background.  The theme will tell us whether or not we should also paint the
  // CSS background.
  IntRect snapped_paint_rect(PixelSnappedIntRect(paint_rect));
  ThemePainter& theme_painter = LayoutTheme::GetTheme().Painter();
  bool theme_painted =
      box_decoration_data.has_appearance &&
      !theme_painter.Paint(layout_box_, paint_info, snapped_paint_rect);
  bool should_paint_background =
      !theme_painted && (!paint_info.SkipRootBackground() ||
                         paint_info.PaintContainer() != &layout_box_);
  if (should_paint_background) {
    PaintBackground(paint_info, paint_rect,
                    box_decoration_data.background_color,
                    box_decoration_data.bleed_avoidance);

    if (box_decoration_data.has_appearance) {
      theme_painter.PaintDecorations(layout_box_.GetNode(),
                                     layout_box_.GetDocument(), style,
                                     paint_info, snapped_paint_rect);
    }
  }

  if (!painting_overflow_contents) {
    BoxPainterBase::PaintInsetBoxShadowWithBorderRect(paint_info, paint_rect,
                                                      style);

    // The theme will tell us whether or not we should also paint the CSS
    // border.
    if (box_decoration_data.has_border_decoration &&
        (!box_decoration_data.has_appearance ||
         (!theme_painted &&
          LayoutTheme::GetTheme().Painter().PaintBorderOnly(
              layout_box_.GetNode(), style, paint_info, snapped_paint_rect))) &&
        !(layout_box_.IsTable() &&
          ToLayoutTable(&layout_box_)->ShouldCollapseBorders())) {
      BoxPainterBase::PaintBorder(
          layout_box_, layout_box_.GetDocument(), layout_box_.GeneratingNode(),
          paint_info, paint_rect, style, box_decoration_data.bleed_avoidance);
    }
  }

  if (needs_end_layer)
    paint_info.context.EndLayer();
}

void BoxPainter::PaintBackground(const PaintInfo& paint_info,
                                 const LayoutRect& paint_rect,
                                 const Color& background_color,
                                 BackgroundBleedAvoidance bleed_avoidance) {
  if (layout_box_.IsDocumentElement())
    return;
  if (layout_box_.BackgroundStolenForBeingBody())
    return;
  if (layout_box_.BackgroundIsKnownToBeObscured())
    return;
  BackgroundImageGeometry geometry(layout_box_);
  BoxModelObjectPainter box_model_painter(layout_box_);
  box_model_painter.PaintFillLayers(paint_info, background_color,
                                    layout_box_.Style()->BackgroundLayers(),
                                    paint_rect, geometry, bleed_avoidance);
}

void BoxPainter::PaintMask(const PaintInfo& paint_info,
                           const LayoutPoint& paint_offset) {
  DCHECK_EQ(PaintPhase::kMask, paint_info.phase);

  if (!layout_box_.HasMask() ||
      layout_box_.Style()->Visibility() != EVisibility::kVisible)
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_box_, paint_info.phase))
    return;

  DrawingRecorder recorder(paint_info.context, layout_box_, paint_info.phase);
  LayoutRect paint_rect = LayoutRect(paint_offset, layout_box_.Size());
  PaintMaskImages(paint_info, paint_rect);
}

void BoxPainter::PaintMaskImages(const PaintInfo& paint_info,
                                 const LayoutRect& paint_rect) {
  // For mask images legacy layout painting handles multi-line boxes by giving
  // the full width of the element, not the current line box, thereby clipping
  // the offending edges.
  bool include_logical_left_edge = true;
  bool include_logical_right_edge = true;

  BackgroundImageGeometry geometry(layout_box_);
  BoxModelObjectPainter painter(layout_box_);
  painter.PaintMaskImages(paint_info, paint_rect, layout_box_, geometry,
                          include_logical_left_edge,
                          include_logical_right_edge);
}

void BoxPainter::PaintClippingMask(const PaintInfo& paint_info,
                                   const LayoutPoint& paint_offset) {
  // SPv175 always paints clipping mask in PaintLayerPainter.
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  DCHECK(paint_info.phase == PaintPhase::kClippingMask);

  if (layout_box_.Style()->Visibility() != EVisibility::kVisible)
    return;

  if (!layout_box_.Layer() ||
      layout_box_.Layer()->GetCompositingState() != kPaintsIntoOwnBacking)
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_box_, paint_info.phase))
    return;

  IntRect paint_rect =
      PixelSnappedIntRect(LayoutRect(paint_offset, layout_box_.Size()));
  DrawingRecorder recorder(paint_info.context, layout_box_, paint_info.phase);
  paint_info.context.FillRect(paint_rect, Color::kBlack);
}

}  // namespace blink
