// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/scrollbar_painter.h"

#include "third_party/blink/renderer/core/layout/layout_scrollbar.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar_part.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"

namespace blink {

void ScrollbarPainter::PaintPart(GraphicsContext& graphics_context,
                                 ScrollbarPart part_type,
                                 const IntRect& rect) {
  const LayoutScrollbarPart* part_layout_object =
      layout_scrollbar_->GetPart(part_type);
  if (!part_layout_object)
    return;
  PaintIntoRect(*part_layout_object, graphics_context,
                layout_scrollbar_->Location(), LayoutRect(rect));
}

void ScrollbarPainter::PaintIntoRect(
    const LayoutScrollbarPart& layout_scrollbar_part,
    GraphicsContext& graphics_context,
    const LayoutPoint& paint_offset,
    const LayoutRect& rect) {
  // Make sure our dimensions match the rect.
  // FIXME: Setting these is a bad layering violation!
  const_cast<LayoutScrollbarPart&>(layout_scrollbar_part)
      .SetLocation(rect.Location() - ToSize(paint_offset));
  const_cast<LayoutScrollbarPart&>(layout_scrollbar_part)
      .SetWidth(rect.Width());
  const_cast<LayoutScrollbarPart&>(layout_scrollbar_part)
      .SetHeight(rect.Height());

  PaintInfo paint_info(graphics_context, PixelSnappedIntRect(rect),
                       PaintPhase::kForeground, kGlobalPaintNormalPhase,
                       kPaintLayerNoFlag);
  ObjectPainter(layout_scrollbar_part)
      .PaintAllPhasesAtomically(paint_info, paint_offset);
}

}  // namespace blink
