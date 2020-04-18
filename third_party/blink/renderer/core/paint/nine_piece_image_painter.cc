// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/nine_piece_image_painter.h"

#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/nine_piece_image_grid.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style/nine_piece_image.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/scoped_interpolation_quality.h"

namespace blink {

namespace {

void PaintPieces(GraphicsContext& context,
                 const LayoutRect& border_image_rect,
                 const ComputedStyle& style,
                 const NinePieceImage& nine_piece_image,
                 Image* image,
                 IntSize image_size,
                 bool include_logical_left_edge,
                 bool include_logical_right_edge) {
  IntRectOutsets border_widths(style.BorderTopWidth(), style.BorderRightWidth(),
                               style.BorderBottomWidth(),
                               style.BorderLeftWidth());
  NinePieceImageGrid grid(
      nine_piece_image, image_size, PixelSnappedIntRect(border_image_rect),
      border_widths, include_logical_left_edge, include_logical_right_edge);

  for (NinePiece piece = kMinPiece; piece < kMaxPiece; ++piece) {
    NinePieceImageGrid::NinePieceDrawInfo draw_info = grid.GetNinePieceDrawInfo(
        piece, nine_piece_image.GetImage()->ImageScaleFactor());

    if (draw_info.is_drawable) {
      if (draw_info.is_corner_piece) {
        // Since there is no way for the developer to specify decode behavior,
        // use kSync by default.
        context.DrawImage(image, Image::kSyncDecode, draw_info.destination,
                          &draw_info.source);
      } else {
        context.DrawTiledImage(image, draw_info.destination, draw_info.source,
                               draw_info.tile_scale,
                               draw_info.tile_rule.horizontal,
                               draw_info.tile_rule.vertical);
      }
    }
  }
}

}  // anonymous namespace

bool NinePieceImagePainter::Paint(GraphicsContext& graphics_context,
                                  const ImageResourceObserver& observer,
                                  const Document& document,
                                  Node* node,
                                  const LayoutRect& rect,
                                  const ComputedStyle& style,
                                  const NinePieceImage& nine_piece_image,
                                  bool include_logical_left_edge,
                                  bool include_logical_right_edge) {
  StyleImage* style_image = nine_piece_image.GetImage();
  if (!style_image)
    return false;

  if (!style_image->IsLoaded())
    return true;  // Never paint a nine-piece image incrementally, but don't
                  // paint the fallback borders either.

  if (!style_image->CanRender())
    return false;

  // FIXME: border-image is broken with full page zooming when tiling has to
  // happen, since the tiling function doesn't have any understanding of the
  // zoom that is in effect on the tile.
  LayoutRect rect_with_outsets = rect;
  rect_with_outsets.Expand(style.ImageOutsets(nine_piece_image));
  LayoutRect border_image_rect = rect_with_outsets;

  // NinePieceImage returns the image slices without effective zoom applied and
  // thus we compute the nine piece grid on top of the image in unzoomed
  // coordinates.
  //
  // FIXME: The default object size passed to imageSize() should be scaled by
  // the zoom factor passed in. In this case it means that borderImageRect
  // should be passed in compensated by effective zoom, since the scale factor
  // is one. For generated images, the actual image data (gradient stops, etc.)
  // are scaled to effective zoom instead so we must take care not to cause
  // scale of them again.
  IntSize image_size = RoundedIntSize(
      style_image->ImageSize(document, 1, border_image_rect.Size()));
  scoped_refptr<Image> image =
      style_image->GetImage(observer, document, style, FloatSize(image_size));

  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "PaintImage",
               "data",
               InspectorPaintImageEvent::Data(node, *style_image, image->Rect(),
                                              FloatRect(border_image_rect)));

  ScopedInterpolationQuality interpolation_quality_scope(
      graphics_context, style.GetInterpolationQuality());
  PaintPieces(graphics_context, border_image_rect, style, nine_piece_image,
              image.get(), image_size, include_logical_left_edge,
              include_logical_right_edge);

  return true;
}

}  // namespace blink
