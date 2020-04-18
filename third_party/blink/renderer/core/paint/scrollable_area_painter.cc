// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/scrollable_area_painter.h"

#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/paint/scrollbar_painter.h"
#include "third_party/blink/renderer/core/paint/transform_recorder.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/platform_chrome_client.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"

namespace blink {

void ScrollableAreaPainter::PaintResizer(GraphicsContext& context,
                                         const IntPoint& paint_offset,
                                         const CullRect& cull_rect) {
  if (GetScrollableArea().GetLayoutBox()->Style()->Resize() == EResize::kNone)
    return;

  IntRect abs_rect = GetScrollableArea().ResizerCornerRect(
      GetScrollableArea().GetLayoutBox()->PixelSnappedBorderBoxRect(
          GetScrollableArea().Layer()->SubpixelAccumulation()),
      kResizerForPointer);
  if (abs_rect.IsEmpty())
    return;
  abs_rect.MoveBy(paint_offset);

  if (const auto* resizer = GetScrollableArea().Resizer()) {
    if (!cull_rect.IntersectsCullRect(abs_rect))
      return;
    ScrollbarPainter::PaintIntoRect(*resizer, context, paint_offset,
                                    LayoutRect(abs_rect));
    return;
  }

  const auto& client = DisplayItemClientForCorner();
  if (DrawingRecorder::UseCachedDrawingIfPossible(context, client,
                                                  DisplayItem::kResizer))
    return;

  DrawingRecorder recorder(context, client, DisplayItem::kResizer);

  DrawPlatformResizerImage(context, abs_rect);

  // Draw a frame around the resizer (1px grey line) if there are any scrollbars
  // present.  Clipping will exclude the right and bottom edges of this frame.
  if (!GetScrollableArea().HasOverlayScrollbars() &&
      GetScrollableArea().HasScrollbar()) {
    GraphicsContextStateSaver state_saver(context);
    context.Clip(abs_rect);
    IntRect larger_corner = abs_rect;
    larger_corner.SetSize(
        IntSize(larger_corner.Width() + 1, larger_corner.Height() + 1));
    context.SetStrokeColor(Color(217, 217, 217));
    context.SetStrokeThickness(1.0f);
    context.SetFillColor(Color::kTransparent);
    context.DrawRect(larger_corner);
  }
}

void ScrollableAreaPainter::DrawPlatformResizerImage(
    GraphicsContext& context,
    IntRect resizer_corner_rect) {
  IntPoint points[4];
  bool on_left = false;
  if (GetScrollableArea()
          .GetLayoutBox()
          ->ShouldPlaceBlockDirectionScrollbarOnLogicalLeft()) {
    on_left = true;
    points[0].SetX(resizer_corner_rect.X() + 1);
    points[1].SetX(resizer_corner_rect.X() + resizer_corner_rect.Width() -
                   resizer_corner_rect.Width() / 2);
    points[2].SetX(points[0].X());
    points[3].SetX(resizer_corner_rect.X() + resizer_corner_rect.Width() -
                   resizer_corner_rect.Width() * 3 / 4);
  } else {
    points[0].SetX(resizer_corner_rect.X() + resizer_corner_rect.Width() - 1);
    points[1].SetX(resizer_corner_rect.X() + resizer_corner_rect.Width() / 2);
    points[2].SetX(points[0].X());
    points[3].SetX(resizer_corner_rect.X() +
                   resizer_corner_rect.Width() * 3 / 4);
  }
  points[0].SetY(resizer_corner_rect.Y() + resizer_corner_rect.Height() / 2);
  points[1].SetY(resizer_corner_rect.Y() + resizer_corner_rect.Height() - 1);
  points[2].SetY(resizer_corner_rect.Y() +
                 resizer_corner_rect.Height() * 3 / 4);
  points[3].SetY(points[1].Y());

  PaintFlags paint_flags;
  paint_flags.setStyle(PaintFlags::kStroke_Style);
  paint_flags.setStrokeWidth(1);

  SkPath line_path;

  // Draw a dark line, to ensure contrast against a light background
  line_path.moveTo(points[0].X(), points[0].Y());
  line_path.lineTo(points[1].X(), points[1].Y());
  line_path.moveTo(points[2].X(), points[2].Y());
  line_path.lineTo(points[3].X(), points[3].Y());
  paint_flags.setColor(SkColorSetARGB(153, 0, 0, 0));
  context.DrawPath(line_path, paint_flags);

  // Draw a light line one pixel below the light line,
  // to ensure contrast against a dark background
  line_path.rewind();
  line_path.moveTo(points[0].X(), points[0].Y() + 1);
  line_path.lineTo(points[1].X() + (on_left ? -1 : 1), points[1].Y());
  line_path.moveTo(points[2].X(), points[2].Y() + 1);
  line_path.lineTo(points[3].X() + (on_left ? -1 : 1), points[3].Y());
  paint_flags.setColor(SkColorSetARGB(153, 255, 255, 255));
  context.DrawPath(line_path, paint_flags);
}

void ScrollableAreaPainter::PaintOverflowControls(
    const PaintInfo& paint_info,
    const IntPoint& paint_offset,
    bool painting_overlay_controls) {
  // Don't do anything if we have no overflow.
  if (!GetScrollableArea().GetLayoutBox()->HasOverflowClip())
    return;

  IntPoint adjusted_paint_offset = paint_offset;
  if (painting_overlay_controls)
    adjusted_paint_offset = GetScrollableArea().CachedOverlayScrollbarOffset();

  CullRect adjusted_cull_rect(paint_info.GetCullRect(), -adjusted_paint_offset);
  // Overlay scrollbars paint in a second pass through the layer tree so that
  // they will paint on top of everything else. If this is the normal painting
  // pass, paintingOverlayControls will be false, and we should just tell the
  // root layer that there are overlay scrollbars that need to be painted. That
  // will cause the second pass through the layer tree to run, and we'll paint
  // the scrollbars then. In the meantime, cache tx and ty so that the second
  // pass doesn't need to re-enter the LayoutTree to get it right.
  if (GetScrollableArea().HasOverlayScrollbars() &&
      !painting_overlay_controls) {
    GetScrollableArea().SetCachedOverlayScrollbarOffset(paint_offset);
    // It's not necessary to do the second pass if the scrollbars paint into
    // layers.
    if ((GetScrollableArea().HorizontalScrollbar() &&
         GetScrollableArea().LayerForHorizontalScrollbar()) ||
        (GetScrollableArea().VerticalScrollbar() &&
         GetScrollableArea().LayerForVerticalScrollbar()))
      return;
    if (!OverflowControlsIntersectRect(adjusted_cull_rect))
      return;

    LayoutView* layout_view = GetScrollableArea().GetLayoutBox()->View();

    PaintLayer* painting_root =
        GetScrollableArea().Layer()->EnclosingLayerWithCompositedLayerMapping(
            kIncludeSelf);
    if (!painting_root)
      painting_root = layout_view->Layer();

    painting_root->SetContainsDirtyOverlayScrollbars(true);
    return;
  }

  // This check is required to avoid painting custom CSS scrollbars twice.
  if (painting_overlay_controls && !GetScrollableArea().HasOverlayScrollbars())
    return;

  GraphicsContext& context = paint_info.context;

  base::Optional<ClipRecorder> clip_recorder;
  base::Optional<ScopedPaintChunkProperties> scoped_paint_chunk_properties;
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    const auto& box = *GetScrollableArea().GetLayoutBox();
    if (const auto* fragment = paint_info.FragmentToPaint(box)) {
      const auto* properties = fragment->PaintProperties();
      // TODO(crbug.com/849278): Remove either the DCHECK or the if condition
      // when we figure out in what cases that the box doesn't have properties.
      DCHECK(properties);
      if (properties) {
        if (const auto* clip = properties->OverflowControlsClip()) {
          scoped_paint_chunk_properties.emplace(
              context.GetPaintController(), clip, box,
              DisplayItem::kClipLayerOverflowControls);
        }
      }
    }
  } else {
    IntRect clip_rect(adjusted_paint_offset,
                      GetScrollableArea().Layer()->PixelSnappedSize());
    clip_recorder.emplace(context, *GetScrollableArea().GetLayoutBox(),
                          DisplayItem::kClipLayerOverflowControls, clip_rect);
  }

  if (GetScrollableArea().HorizontalScrollbar() &&
      !GetScrollableArea().LayerForHorizontalScrollbar()) {
    TransformRecorder translate_recorder(
        context, *GetScrollableArea().HorizontalScrollbar(),
        AffineTransform::Translation(adjusted_paint_offset.X(),
                                     adjusted_paint_offset.Y()));
    GetScrollableArea().HorizontalScrollbar()->Paint(context,
                                                     adjusted_cull_rect);
  }
  if (GetScrollableArea().VerticalScrollbar() &&
      !GetScrollableArea().LayerForVerticalScrollbar()) {
    TransformRecorder translate_recorder(
        context, *GetScrollableArea().VerticalScrollbar(),
        AffineTransform::Translation(adjusted_paint_offset.X(),
                                     adjusted_paint_offset.Y()));
    GetScrollableArea().VerticalScrollbar()->Paint(context, adjusted_cull_rect);
  }
  if (!GetScrollableArea().LayerForScrollCorner()) {
    // We fill our scroll corner with white if we have a scrollbar that doesn't
    // run all the way up to the edge of the box.
    PaintScrollCorner(context, adjusted_paint_offset, paint_info.GetCullRect());

    // Paint our resizer last, since it sits on top of the scroll corner.
    PaintResizer(context, adjusted_paint_offset, paint_info.GetCullRect());
  }
}

bool ScrollableAreaPainter::OverflowControlsIntersectRect(
    const CullRect& cull_rect) const {
  const IntRect border_box =
      GetScrollableArea().GetLayoutBox()->PixelSnappedBorderBoxRect(
          GetScrollableArea().Layer()->SubpixelAccumulation());

  if (cull_rect.IntersectsCullRect(
          GetScrollableArea().RectForHorizontalScrollbar(border_box)))
    return true;

  if (cull_rect.IntersectsCullRect(
          GetScrollableArea().RectForVerticalScrollbar(border_box)))
    return true;

  if (cull_rect.IntersectsCullRect(GetScrollableArea().ScrollCornerRect()))
    return true;

  if (cull_rect.IntersectsCullRect(GetScrollableArea().ResizerCornerRect(
          border_box, kResizerForPointer)))
    return true;

  return false;
}

void ScrollableAreaPainter::PaintScrollCorner(
    GraphicsContext& context,
    const IntPoint& paint_offset,
    const CullRect& adjusted_cull_rect) {
  IntRect abs_rect = GetScrollableArea().ScrollCornerRect();
  if (abs_rect.IsEmpty())
    return;
  abs_rect.MoveBy(paint_offset);

  if (const auto* scroll_corner = GetScrollableArea().ScrollCorner()) {
    if (!adjusted_cull_rect.IntersectsCullRect(abs_rect))
      return;
    ScrollbarPainter::PaintIntoRect(*scroll_corner, context, paint_offset,
                                    LayoutRect(abs_rect));
    return;
  }

  // We don't want to paint opaque if we have overlay scrollbars, since we need
  // to see what is behind it.
  if (GetScrollableArea().HasOverlayScrollbars())
    return;

  ScrollbarTheme* theme = nullptr;

  if (GetScrollableArea().HorizontalScrollbar()) {
    theme = &GetScrollableArea().HorizontalScrollbar()->GetTheme();
  } else if (GetScrollableArea().VerticalScrollbar()) {
    theme = &GetScrollableArea().VerticalScrollbar()->GetTheme();
  } else {
    NOTREACHED();
  }

  const auto& client = DisplayItemClientForCorner();
  theme->PaintScrollCorner(context, client, abs_rect);
}

PaintLayerScrollableArea& ScrollableAreaPainter::GetScrollableArea() const {
  return *scrollable_area_;
}

const DisplayItemClient& ScrollableAreaPainter::DisplayItemClientForCorner()
    const {
  if (const auto* graphics_layer = GetScrollableArea().LayerForScrollCorner())
    return *graphics_layer;
  return *GetScrollableArea().GetLayoutBox();
}

}  // namespace blink
