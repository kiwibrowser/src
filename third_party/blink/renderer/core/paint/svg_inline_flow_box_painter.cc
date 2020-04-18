// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/svg_inline_flow_box_painter.h"

#include "third_party/blink/renderer/core/layout/api/line_layout_api_shim.h"
#include "third_party/blink/renderer/core/layout/svg/line/svg_inline_flow_box.h"
#include "third_party/blink/renderer/core/layout/svg/line/svg_inline_text_box.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/svg_inline_text_box_painter.h"
#include "third_party/blink/renderer/core/paint/svg_paint_context.h"

namespace blink {

void SVGInlineFlowBoxPainter::PaintSelectionBackground(
    const PaintInfo& paint_info) {
  DCHECK(paint_info.phase == PaintPhase::kForeground ||
         paint_info.phase == PaintPhase::kSelection);

  PaintInfo child_paint_info(paint_info);
  for (InlineBox* child = svg_inline_flow_box_.FirstChild(); child;
       child = child->NextOnLine()) {
    if (child->IsSVGInlineTextBox())
      SVGInlineTextBoxPainter(*ToSVGInlineTextBox(child))
          .PaintSelectionBackground(child_paint_info);
    else if (child->IsSVGInlineFlowBox())
      SVGInlineFlowBoxPainter(*ToSVGInlineFlowBox(child))
          .PaintSelectionBackground(child_paint_info);
  }
}

void SVGInlineFlowBoxPainter::Paint(const PaintInfo& paint_info,
                                    const LayoutPoint& paint_offset) {
  DCHECK(paint_info.phase == PaintPhase::kForeground ||
         paint_info.phase == PaintPhase::kSelection);

  SVGPaintContext paint_context(*LineLayoutAPIShim::ConstLayoutObjectFrom(
                                    svg_inline_flow_box_.GetLineLayoutItem()),
                                paint_info);
  if (paint_context.ApplyClipMaskAndFilterIfNecessary()) {
    for (InlineBox* child = svg_inline_flow_box_.FirstChild(); child;
         child = child->NextOnLine())
      child->Paint(paint_context.GetPaintInfo(), paint_offset, LayoutUnit(),
                   LayoutUnit());
  }
}

}  // namespace blink
