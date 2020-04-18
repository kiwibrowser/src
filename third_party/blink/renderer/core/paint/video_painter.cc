// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/video_painter.h"

#include "cc/layers/layer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/layout/layout_video.h"
#include "third_party/blink/renderer/core/paint/image_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/foreign_layer_display_item.h"

namespace blink {

void VideoPainter::PaintReplaced(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  WebMediaPlayer* media_player =
      layout_video_.MediaElement()->GetWebMediaPlayer();
  bool displaying_poster =
      layout_video_.VideoElement()->ShouldDisplayPosterImage();
  if (!displaying_poster && !media_player)
    return;

  LayoutRect replaced_rect(layout_video_.ReplacedContentRect());
  replaced_rect.MoveBy(paint_offset);
  IntRect snapped_replaced_rect = PixelSnappedIntRect(replaced_rect);

  if (snapped_replaced_rect.IsEmpty())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_video_, paint_info.phase))
    return;

  GraphicsContext& context = paint_info.context;
  LayoutRect content_rect = layout_video_.ContentBoxRect();
  content_rect.MoveBy(paint_offset);

  // Video frames are only painted in software for printing or capturing node
  // images via web APIs.
  bool force_software_video_paint =
      paint_info.GetGlobalPaintFlags() & kGlobalPaintFlattenCompositingLayers;

  bool paint_with_foreign_layer =
      !displaying_poster && !force_software_video_paint &&
      RuntimeEnabledFeatures::SlimmingPaintV2Enabled();
  if (paint_with_foreign_layer) {
    if (cc::Layer* layer = layout_video_.MediaElement()->CcLayer()) {
      IntRect pixel_snapped_rect = PixelSnappedIntRect(content_rect);
      layer->SetBounds(static_cast<gfx::Size>(pixel_snapped_rect.Size()));
      layer->SetIsDrawable(true);
      RecordForeignLayer(
          context, layout_video_, DisplayItem::kForeignLayerVideo, layer,
          pixel_snapped_rect.Location(), pixel_snapped_rect.Size());
      return;
    }
  }

  // TODO(trchen): Video rect could overflow the content rect due to object-fit.
  // Should apply a clip here like EmbeddedObjectPainter does.
  DrawingRecorder recorder(context, layout_video_, paint_info.phase);

  if (displaying_poster || !force_software_video_paint) {
    // This will display the poster image, if one is present, and otherwise
    // paint nothing.
    ImagePainter(layout_video_)
        .PaintIntoRect(context, replaced_rect, content_rect);
  } else {
    PaintFlags video_flags = context.FillFlags();
    video_flags.setColor(SK_ColorBLACK);
    layout_video_.VideoElement()->PaintCurrentFrame(
        context.Canvas(), snapped_replaced_rect, &video_flags);
  }
}

}  // namespace blink
