// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_IMAGE_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_IMAGE_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class GraphicsContext;
struct PaintInfo;
class LayoutPoint;
class LayoutRect;
class LayoutImage;

class ImagePainter {
  STACK_ALLOCATED();

 public:
  ImagePainter(const LayoutImage& layout_image) : layout_image_(layout_image) {}

  void Paint(const PaintInfo&, const LayoutPoint& paint_offset);
  void PaintReplaced(const PaintInfo&, const LayoutPoint& paint_offset);

  // Paint the image into |destRect|, after clipping by |contentRect|. Both
  // |destRect| and |contentRect| should be in local coordinates plus the paint
  // offset.
  void PaintIntoRect(GraphicsContext&,
                     const LayoutRect& dest_rect,
                     const LayoutRect& content_rect);

 private:
  void PaintAreaElementFocusRing(const PaintInfo&,
                                 const LayoutPoint& paint_offset);

  const LayoutImage& layout_image_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_IMAGE_PAINTER_H_
