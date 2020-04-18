// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/view_painter.h"

#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/background_image_geometry.h"
#include "third_party/blink/renderer/core/paint/block_painter.h"
#include "third_party/blink/renderer/core/paint/box_model_object_painter.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/scroll_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

void ViewPainter::Paint(const PaintInfo& paint_info,
                        const LayoutPoint& paint_offset) {
  // If we ever require layout but receive a paint anyway, something has gone
  // horribly wrong.
  DCHECK(!layout_view_.NeedsLayout());
  // LayoutViews should never be called to paint with an offset not on device
  // pixels.
  DCHECK(LayoutPoint(IntPoint(paint_offset.X().ToInt(),
                              paint_offset.Y().ToInt())) == paint_offset);

  DCHECK(!layout_view_.GetFrameView()->ShouldThrottleRendering());

  BlockPainter(layout_view_).Paint(paint_info, paint_offset);
}

void ViewPainter::PaintBoxDecorationBackground(const PaintInfo& paint_info) {
  if (paint_info.SkipRootBackground())
    return;

  // This function overrides background painting for the LayoutView.
  // View background painting is special in the following ways:
  // 1. The view paints background for the root element, the background
  //    positioning respects the positioning and transformation of the root
  //    element.
  // 2. CSS background-clip is ignored, the background layers always expand to
  //    cover the whole canvas. None of the stacking context effects (except
  //    transformation) on the root element affects the background.
  // 3. The main frame is also responsible for painting the user-agent-defined
  //    base background color. Conceptually it should be painted by the embedder
  //    but painting it here allows culling and pre-blending optimization when
  //    possible.

  GraphicsContext& context = paint_info.context;

  // The background rect always includes at least the visible content size.
  IntRect background_rect(
      PixelSnappedIntRect(layout_view_.OverflowClipRect(LayoutPoint())));

  // When printing, paint the entire unclipped scrolling content area.
  if (paint_info.IsPrinting())
    background_rect.Unite(layout_view_.DocumentRect());

  const DisplayItemClient* display_item_client = &layout_view_;

  base::Optional<ScopedPaintChunkProperties> scoped_scroll_property;
  base::Optional<ScrollRecorder> scroll_recorder;
  if (BoxModelObjectPainter::
          IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
              &layout_view_, paint_info)) {
    // Layout overflow, combined with the visible content size.
    background_rect.Unite(layout_view_.DocumentRect());
    display_item_client = layout_view_.Layer()->GraphicsLayerBacking();
    if (!layout_view_.ScrolledContentOffset().IsZero()) {
      scroll_recorder.emplace(paint_info.context, layout_view_,
                              paint_info.phase,
                              layout_view_.ScrolledContentOffset());
    }
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      scoped_scroll_property.emplace(
          paint_info.context.GetPaintController(),
          layout_view_.FirstFragment().ContentsProperties(),
          *display_item_client, DisplayItem::kDocumentBackground);
    }
  }

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, *display_item_client, DisplayItem::kDocumentBackground))
    return;

  const Document& document = layout_view_.GetDocument();
  const LocalFrameView& frame_view = *layout_view_.GetFrameView();
  bool is_main_frame = document.IsInMainFrame();
  bool paints_base_background =
      is_main_frame && (frame_view.BaseBackgroundColor().Alpha() > 0);
  bool should_clear_canvas =
      paints_base_background &&
      (document.GetSettings() &&
       document.GetSettings()->GetShouldClearDocumentBackground());
  Color base_background_color =
      paints_base_background ? frame_view.BaseBackgroundColor() : Color();
  Color root_background_color = layout_view_.Style()->VisitedDependentColor(
      GetCSSPropertyBackgroundColor());
  const LayoutObject* root_object =
      document.documentElement() ? document.documentElement()->GetLayoutObject()
                                 : nullptr;

  DrawingRecorder recorder(context, *display_item_client,
                           DisplayItem::kDocumentBackground);

  // Special handling for print economy mode.
  bool force_background_to_white =
      BoxModelObjectPainter::ShouldForceWhiteBackgroundForPrintEconomy(
          document, layout_view_.StyleRef());
  if (force_background_to_white) {
    // If for any reason the view background is not transparent, paint white
    // instead, otherwise keep transparent as is.
    if (paints_base_background || root_background_color.Alpha() ||
        layout_view_.Style()->BackgroundLayers().GetImage())
      context.FillRect(background_rect, Color::kWhite, SkBlendMode::kSrc);
    return;
  }

  // Compute the enclosing rect of the view, in root element space.
  //
  // For background colors we can simply paint the document rect in the default
  // space.  However for background image, the root element transform applies.
  // The strategy is to apply root element transform on the context and issue
  // draw commands in the local space, therefore we need to apply inverse
  // transform on the document rect to get to the root element space.
  // Local / scroll positioned background images will be painted into scrolling
  // contents layer with root layer scrolling. Therefore we need to switch both
  // the background_rect and context to documentElement visual space.
  bool background_renderable = true;
  TransformationMatrix transform;
  IntRect paint_rect = background_rect;
  if (!root_object || !root_object->IsBox()) {
    background_renderable = false;
  } else if (root_object->HasLayer()) {
    if (BoxModelObjectPainter::
            IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
                &layout_view_, paint_info)) {
      transform.Translate(layout_view_.ScrolledContentOffset().Width(),
                          layout_view_.ScrolledContentOffset().Height());
    }
    const PaintLayer& root_layer =
        *ToLayoutBoxModelObject(root_object)->Layer();
    LayoutPoint offset;
    root_layer.ConvertToLayerCoords(nullptr, offset);
    transform.Translate(offset.X(), offset.Y());
    transform.Multiply(
        root_layer.RenderableTransform(paint_info.GetGlobalPaintFlags()));

    if (!transform.IsInvertible()) {
      background_renderable = false;
    } else {
      bool is_clamped;
      paint_rect = transform.Inverse()
                       .ProjectQuad(FloatQuad(background_rect), &is_clamped)
                       .EnclosingBoundingBox();
      background_renderable = !is_clamped;
    }
  }

  if (!background_renderable) {
    if (base_background_color.Alpha()) {
      context.FillRect(
          background_rect, base_background_color,
          should_clear_canvas ? SkBlendMode::kSrc : SkBlendMode::kSrcOver);
    } else if (should_clear_canvas) {
      context.FillRect(background_rect, Color(), SkBlendMode::kClear);
    }
    return;
  }

  BoxPainterBase::FillLayerOcclusionOutputList reversed_paint_list;
  bool should_draw_background_in_separate_buffer =
      BoxModelObjectPainter(layout_view_)
          .CalculateFillLayerOcclusionCulling(
              reversed_paint_list, layout_view_.Style()->BackgroundLayers());
  DCHECK(reversed_paint_list.size());

  // If the root background color is opaque, isolation group can be skipped
  // because the canvas
  // will be cleared by root background color.
  if (!root_background_color.HasAlpha())
    should_draw_background_in_separate_buffer = false;

  // We are going to clear the canvas with transparent pixels, isolation group
  // can be skipped.
  if (!base_background_color.Alpha() && should_clear_canvas)
    should_draw_background_in_separate_buffer = false;

  if (should_draw_background_in_separate_buffer) {
    if (base_background_color.Alpha()) {
      context.FillRect(
          background_rect, base_background_color,
          should_clear_canvas ? SkBlendMode::kSrc : SkBlendMode::kSrcOver);
    }
    context.BeginLayer();
  }

  Color combined_background_color =
      should_draw_background_in_separate_buffer
          ? root_background_color
          : base_background_color.Blend(root_background_color);

  if (combined_background_color != frame_view.BaseBackgroundColor())
    context.GetPaintController().SetFirstPainted();

  if (combined_background_color.Alpha()) {
    if (!combined_background_color.HasAlpha() &&
        RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      recorder.SetKnownToBeOpaque();
    context.FillRect(
        background_rect, combined_background_color,
        (should_draw_background_in_separate_buffer || should_clear_canvas)
            ? SkBlendMode::kSrc
            : SkBlendMode::kSrcOver);
  } else if (should_clear_canvas &&
             !should_draw_background_in_separate_buffer) {
    context.FillRect(background_rect, Color(), SkBlendMode::kClear);
  }

  BackgroundImageGeometry geometry(layout_view_);
  BoxModelObjectPainter box_model_painter(layout_view_);
  for (auto it = reversed_paint_list.rbegin(); it != reversed_paint_list.rend();
       ++it) {
    DCHECK((*it)->Clip() == EFillBox::kBorder);

    bool should_paint_in_viewport_space =
        (*it)->Attachment() == EFillAttachment::kFixed;
    if (should_paint_in_viewport_space) {
      box_model_painter.PaintFillLayer(paint_info, Color(), **it,
                                       LayoutRect(background_rect),
                                       kBackgroundBleedNone, geometry);
    } else {
      context.Save();
      // TODO(trchen): We should be able to handle 3D-transformed root
      // background with slimming paint by using transform display items.
      context.ConcatCTM(transform.ToAffineTransform());
      box_model_painter.PaintFillLayer(paint_info, Color(), **it,
                                       LayoutRect(paint_rect),
                                       kBackgroundBleedNone, geometry);
      context.Restore();
    }
  }

  if (should_draw_background_in_separate_buffer)
    context.EndLayer();
}

}  // namespace blink
