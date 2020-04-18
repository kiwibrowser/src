// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/svg_text_painter.h"

#include "third_party/blink/renderer/core/layout/svg/layout_svg_text.h"
#include "third_party/blink/renderer/core/paint/block_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/svg_paint_context.h"

namespace blink {

void SVGTextPainter::Paint(const PaintInfo& paint_info) {
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection)
    return;

  PaintInfo block_info(paint_info);
  block_info.UpdateCullRect(layout_svg_text_.LocalToSVGParentTransform());
  SVGTransformContext transform_context(
      block_info, layout_svg_text_,
      layout_svg_text_.LocalToSVGParentTransform());

  BlockPainter(layout_svg_text_).Paint(block_info, LayoutPoint());

  // Paint the outlines, if any
  if (paint_info.phase == PaintPhase::kForeground) {
    block_info.phase = PaintPhase::kOutline;
    BlockPainter(layout_svg_text_).Paint(block_info, LayoutPoint());
  }
}

}  // namespace blink
