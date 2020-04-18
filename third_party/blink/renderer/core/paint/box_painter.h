// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BOX_PAINTER_H_

#include "third_party/blink/renderer/core/layout/background_bleed_avoidance.h"
#include "third_party/blink/renderer/core/paint/rounded_inner_rect_clipper.h"
#include "third_party/blink/renderer/platform/geometry/layout_size.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutPoint;
class LayoutRect;
struct PaintInfo;
class LayoutBox;
class Color;

class BoxPainter {
  STACK_ALLOCATED();

 public:
  BoxPainter(const LayoutBox& layout_box) : layout_box_(layout_box) {}
  void Paint(const PaintInfo&, const LayoutPoint&);

  void PaintChildren(const PaintInfo&, const LayoutPoint&);
  void PaintBoxDecorationBackground(const PaintInfo&, const LayoutPoint&);
  void PaintMask(const PaintInfo&, const LayoutPoint&);
  void PaintClippingMask(const PaintInfo&, const LayoutPoint&);

  void PaintMaskImages(const PaintInfo&, const LayoutRect&);
  void PaintBoxDecorationBackgroundWithRect(const PaintInfo&,
                                            const LayoutPoint&,
                                            const LayoutRect&);
  LayoutRect BoundsForDrawingRecorder(const PaintInfo&,
                                      const LayoutPoint& adjusted_paint_offset);

 private:
  void PaintBackground(const PaintInfo&,
                       const LayoutRect&,
                       const Color& background_color,
                       BackgroundBleedAvoidance = kBackgroundBleedNone);

  const LayoutBox& layout_box_;
};

}  // namespace blink

#endif
