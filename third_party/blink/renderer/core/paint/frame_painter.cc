// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/frame_painter.h"

#include "third_party/blink/renderer/core/editing/markers/document_marker_controller.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar_part.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/frame_paint_timing.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"
#include "third_party/blink/renderer/core/paint/scrollbar_painter.h"
#include "third_party/blink/renderer/core/paint/transform_recorder.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"

namespace blink {

bool FramePainter::in_paint_contents_ = false;

void FramePainter::Paint(GraphicsContext& context,
                         const GlobalPaintFlags global_paint_flags,
                         const CullRect& rect) {
  if (GetFrameView().ShouldThrottleRendering())
    return;

  GetFrameView().NotifyPageThatContentAreaWillPaint();

  IntRect document_dirty_rect;
  IntPoint frame_view_location(GetFrameView().Location());
  IntRect visible_area_without_scrollbars(frame_view_location,
                                          GetFrameView().VisibleContentSize());
  IntPoint content_offset =
      -frame_view_location + GetFrameView().ScrollOffsetInt();
  document_dirty_rect = rect.rect_;
  document_dirty_rect.Intersect(visible_area_without_scrollbars);
  document_dirty_rect.MoveBy(content_offset);

  bool should_paint_contents = !document_dirty_rect.IsEmpty();
  bool should_paint_scrollbars = !GetFrameView().ScrollbarsSuppressed() &&
                                 (GetFrameView().HorizontalScrollbar() ||
                                  GetFrameView().VerticalScrollbar());
  if (!should_paint_contents && !should_paint_scrollbars)
    return;

  if (should_paint_contents) {
    TransformRecorder transform_recorder(
        context, *GetFrameView().GetLayoutView(),
        AffineTransform::Translation(
            frame_view_location.X() - GetFrameView().ScrollX(),
            frame_view_location.Y() - GetFrameView().ScrollY()));

    ClipRecorder clip_recorder(context, *GetFrameView().GetLayoutView(),
                               DisplayItem::kClipFrameToVisibleContentRect,
                               GetFrameView().VisibleContentRect());
    PaintContents(context, global_paint_flags, document_dirty_rect);
  }
}

void FramePainter::PaintContents(GraphicsContext& context,
                                 const GlobalPaintFlags global_paint_flags,
                                 const IntRect& rect) {
  Document* document = GetFrameView().GetFrame().GetDocument();

  if (GetFrameView().ShouldThrottleRendering() || !document->IsActive())
    return;

  LayoutView* layout_view = GetFrameView().GetLayoutView();
  if (!layout_view) {
    DLOG(ERROR) << "called FramePainter::paint with nil layoutObject";
    return;
  }

  // TODO(crbug.com/590856): It's still broken when we choose not to crash when
  // the check fails.
  if (!GetFrameView().CheckDoesNotNeedLayout())
    return;

  // TODO(wangxianzhu): The following check should be stricter, but currently
  // this is blocked by the svg root issue (crbug.com/442939).
  DCHECK(document->Lifecycle().GetState() >=
         DocumentLifecycle::kCompositingClean);

  FramePaintTiming frame_paint_timing(context, &GetFrameView().GetFrame());
  TRACE_EVENT1(
      "devtools.timeline,rail", "Paint", "data",
      InspectorPaintEvent::Data(layout_view, LayoutRect(rect), nullptr));

  bool is_top_level_painter = !in_paint_contents_;
  in_paint_contents_ = true;

  FontCachePurgePreventer font_cache_purge_preventer;

  // TODO(jchaffraix): GlobalPaintFlags should be const during a paint
  // phase. Thus we should set this flag upfront (crbug.com/510280).
  GlobalPaintFlags updated_global_paint_flags = global_paint_flags;
  PaintLayerFlags root_layer_paint_flags = 0;
  if (document->Printing()) {
    updated_global_paint_flags |=
        kGlobalPaintFlattenCompositingLayers | kGlobalPaintPrinting;
    // This will prevent clipping the root PaintLayer to its visible content
    // rect when root layer scrolling is enabled.
    root_layer_paint_flags = kPaintLayerPaintingOverflowContents;
  }

  PaintLayer* root_layer = layout_view->Layer();

#if DCHECK_IS_ON()
  layout_view->AssertSubtreeIsLaidOut();
  LayoutObject::SetLayoutNeededForbiddenScope forbid_set_needs_layout(
      root_layer->GetLayoutObject());
#endif

  PaintLayerPainter layer_painter(*root_layer);

  float device_scale_factor = blink::DeviceScaleFactorDeprecated(
      root_layer->GetLayoutObject().GetFrame());
  context.SetDeviceScaleFactor(device_scale_factor);

  layer_painter.Paint(context, LayoutRect(rect), updated_global_paint_flags,
                      root_layer_paint_flags);

  if (root_layer->ContainsDirtyOverlayScrollbars()) {
    layer_painter.PaintOverlayScrollbars(context, LayoutRect(rect),
                                         updated_global_paint_flags);
  }

  // Regions may have changed as a result of the visibility/z-index of element
  // changing.
  if (document->AnnotatedRegionsDirty())
    GetFrameView().UpdateDocumentAnnotatedRegions();

  if (is_top_level_painter) {
    // Everything that happens after paintContents completions is considered
    // to be part of the next frame.
    GetMemoryCache()->UpdateFramePaintTimestamp();
    in_paint_contents_ = false;
  }

  probe::didPaint(layout_view->GetFrame(), nullptr, context, LayoutRect(rect));
}

void FramePainter::PaintScrollbars(GraphicsContext& context,
                                   const IntRect& rect) {
  if (GetFrameView().HorizontalScrollbar() &&
      !GetFrameView().LayerForHorizontalScrollbar())
    PaintScrollbar(context, *GetFrameView().HorizontalScrollbar(), rect);

  if (GetFrameView().VerticalScrollbar() &&
      !GetFrameView().LayerForVerticalScrollbar())
    PaintScrollbar(context, *GetFrameView().VerticalScrollbar(), rect);

  if (!GetFrameView().LayerForScrollCorner() &&
      GetFrameView().IsScrollCornerVisible())
    PaintScrollCorner(context, GetFrameView().ScrollCornerRect());
}

void FramePainter::PaintScrollCorner(GraphicsContext& context,
                                     const IntRect& corner_rect) {
  if (const auto* scroll_corner = GetFrameView().ScrollCorner()) {
    bool needs_background = GetFrameView().GetFrame().IsMainFrame();
    if (needs_background &&
        !DrawingRecorder::UseCachedDrawingIfPossible(
            context, *scroll_corner, DisplayItem::kScrollbarBackground)) {
      DrawingRecorder recorder(context, *scroll_corner,
                               DisplayItem::kScrollbarBackground);
      context.FillRect(corner_rect, GetFrameView().BaseBackgroundColor());
    }
    ScrollbarPainter::PaintIntoRect(*scroll_corner, context,
                                    corner_rect.Location(),
                                    LayoutRect(corner_rect));
    return;
  }

  ScrollbarTheme* theme = nullptr;

  if (GetFrameView().HorizontalScrollbar()) {
    theme = &GetFrameView().HorizontalScrollbar()->GetTheme();
  } else if (GetFrameView().VerticalScrollbar()) {
    theme = &GetFrameView().VerticalScrollbar()->GetTheme();
  } else {
    NOTREACHED();
  }

  const DisplayItemClient* client;
  if (auto* graphics_layer = GetFrameView().LayerForScrollCorner())
    client = graphics_layer;
  else
    client = GetFrameView().GetLayoutView();
  theme->PaintScrollCorner(context, *client, corner_rect);
}

void FramePainter::PaintScrollbar(GraphicsContext& context,
                                  Scrollbar& bar,
                                  const IntRect& rect) {
  bool needs_background =
      bar.IsCustomScrollbar() && GetFrameView().GetFrame().IsMainFrame();
  if (needs_background) {
    IntRect to_fill = bar.FrameRect();
    to_fill.Intersect(rect);
    if (!to_fill.IsEmpty() &&
        !DrawingRecorder::UseCachedDrawingIfPossible(
            context, bar, DisplayItem::kScrollbarBackground)) {
      DrawingRecorder recorder(context, bar, DisplayItem::kScrollbarBackground);
      context.FillRect(to_fill, GetFrameView().BaseBackgroundColor());
    }
  }

  bar.Paint(context, CullRect(rect));
}

const LocalFrameView& FramePainter::GetFrameView() {
  DCHECK(frame_view_);
  return *frame_view_;
}

}  // namespace blink
