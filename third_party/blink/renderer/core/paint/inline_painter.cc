// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/inline_painter.h"

#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/paint/line_box_list_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"

namespace blink {

void InlinePainter::Paint(const PaintInfo& paint_info,
                          const LayoutPoint& paint_offset) {
  if (paint_info.phase == PaintPhase::kForeground && paint_info.IsPrinting())
    ObjectPainter(layout_inline_)
        .AddPDFURLRectIfNeeded(paint_info, paint_offset);

  if (ShouldPaintSelfOutline(paint_info.phase) ||
      ShouldPaintDescendantOutlines(paint_info.phase)) {
    ObjectPainter painter(layout_inline_);
    if (ShouldPaintDescendantOutlines(paint_info.phase))
      painter.PaintInlineChildrenOutlines(paint_info, paint_offset);
    if (ShouldPaintSelfOutline(paint_info.phase) &&
        !layout_inline_.IsElementContinuation())
      painter.PaintOutline(paint_info, paint_offset);
    return;
  }

  LineBoxListPainter(*layout_inline_.LineBoxes())
      .Paint(layout_inline_, paint_info, paint_offset);
}

}  // namespace blink
