// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/fieldset_painter.h"

#include "third_party/blink/renderer/core/layout/layout_fieldset.h"
#include "third_party/blink/renderer/core/paint/background_image_geometry.h"
#include "third_party/blink/renderer/core/paint/box_decoration_data.h"
#include "third_party/blink/renderer/core/paint/box_model_object_painter.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"

namespace blink {

void FieldsetPainter::PaintBoxDecorationBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutRect paint_rect(paint_offset, layout_fieldset_.Size());
  LayoutBox* legend = layout_fieldset_.FindInFlowLegend();
  if (!legend)
    return BoxPainter(layout_fieldset_)
        .PaintBoxDecorationBackground(paint_info, paint_offset);

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_fieldset_, paint_info.phase))
    return;

  // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
  // cases the legend is embedded in the right and bottom borders respectively.
  // https://bugs.webkit.org/show_bug.cgi?id=47236
  if (layout_fieldset_.Style()->IsHorizontalWritingMode()) {
    LayoutUnit y_off =
        (legend->Location().Y() > 0)
            ? LayoutUnit()
            : (legend->Size().Height() - layout_fieldset_.BorderTop()) / 2;
    paint_rect.SetHeight(paint_rect.Height() - y_off);
    paint_rect.SetY(paint_rect.Y() + y_off);
  } else {
    LayoutUnit x_off =
        (legend->Location().X() > 0)
            ? LayoutUnit()
            : (legend->Size().Width() - layout_fieldset_.BorderLeft()) / 2;
    paint_rect.SetWidth(paint_rect.Width() - x_off);
    paint_rect.SetX(paint_rect.X() + x_off);
  }

  DrawingRecorder recorder(paint_info.context, layout_fieldset_,
                           paint_info.phase);
  BoxDecorationData box_decoration_data(layout_fieldset_);

  BoxPainterBase::PaintNormalBoxShadow(paint_info, paint_rect,
                                       layout_fieldset_.StyleRef());
  BackgroundImageGeometry geometry(layout_fieldset_);
  BoxModelObjectPainter(layout_fieldset_)
      .PaintFillLayers(paint_info, box_decoration_data.background_color,
                       layout_fieldset_.Style()->BackgroundLayers(), paint_rect,
                       geometry);
  BoxPainterBase::PaintInsetBoxShadowWithBorderRect(
      paint_info, paint_rect, layout_fieldset_.StyleRef());

  if (!box_decoration_data.has_border_decoration)
    return;

  // Create a clipping region around the legend and paint the border as normal
  GraphicsContext& graphics_context = paint_info.context;
  GraphicsContextStateSaver state_saver(graphics_context);

  // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
  // cases the legend is embedded in the right and bottom borders respectively.
  // https://bugs.webkit.org/show_bug.cgi?id=47236
  if (layout_fieldset_.Style()->IsHorizontalWritingMode()) {
    LayoutUnit clip_top = paint_rect.Y();
    LayoutUnit clip_height =
        max(static_cast<LayoutUnit>(layout_fieldset_.Style()->BorderTopWidth()),
            legend->Size().Height() -
                ((legend->Size().Height() - layout_fieldset_.BorderTop()) / 2));
    graphics_context.ClipOut(
        PixelSnappedIntRect(paint_rect.X() + legend->Location().X(), clip_top,
                            legend->Size().Width(), clip_height));
  } else {
    LayoutUnit clip_left = paint_rect.X();
    LayoutUnit clip_width = max(
        static_cast<LayoutUnit>(layout_fieldset_.Style()->BorderLeftWidth()),
        legend->Size().Width());
    graphics_context.ClipOut(
        PixelSnappedIntRect(clip_left, paint_rect.Y() + legend->Location().Y(),
                            clip_width, legend->Size().Height()));
  }

  Node* node = nullptr;
  const LayoutObject* layout_object = &layout_fieldset_;
  for (; layout_object && !node; layout_object = layout_object->Parent())
    node = layout_object->GeneratingNode();
  BoxPainterBase::PaintBorder(layout_fieldset_, layout_fieldset_.GetDocument(),
                              node, paint_info, paint_rect,
                              layout_fieldset_.StyleRef());
}

void FieldsetPainter::PaintMask(const PaintInfo& paint_info,
                                const LayoutPoint& paint_offset) {
  if (layout_fieldset_.Style()->Visibility() != EVisibility::kVisible ||
      paint_info.phase != PaintPhase::kMask)
    return;

  LayoutRect paint_rect = LayoutRect(paint_offset, layout_fieldset_.Size());
  LayoutBox* legend = layout_fieldset_.FindInFlowLegend();
  if (!legend)
    return BoxPainter(layout_fieldset_).PaintMask(paint_info, paint_offset);

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_fieldset_, paint_info.phase))
    return;

  // FIXME: We need to work with "rl" and "bt" block flow directions.  In those
  // cases the legend is embedded in the right and bottom borders respectively.
  // https://bugs.webkit.org/show_bug.cgi?id=47236
  if (layout_fieldset_.Style()->IsHorizontalWritingMode()) {
    LayoutUnit y_off =
        (legend->Location().Y() > LayoutUnit())
            ? LayoutUnit()
            : (legend->Size().Height() - layout_fieldset_.BorderTop()) / 2;
    paint_rect.Expand(LayoutUnit(), -y_off);
    paint_rect.Move(LayoutUnit(), y_off);
  } else {
    LayoutUnit x_off =
        (legend->Location().X() > LayoutUnit())
            ? LayoutUnit()
            : (legend->Size().Width() - layout_fieldset_.BorderLeft()) / 2;
    paint_rect.Expand(-x_off, LayoutUnit());
    paint_rect.Move(x_off, LayoutUnit());
  }

  DrawingRecorder recorder(paint_info.context, layout_fieldset_,
                           paint_info.phase);
  BoxPainter(layout_fieldset_).PaintMaskImages(paint_info, paint_rect);
}

}  // namespace blink
