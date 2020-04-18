// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/ng/ng_fragment_painter.h"

#include "third_party/blink/renderer/core/layout/layout_theme.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_fragment_traversal.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_outline_utils.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment_traversal.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/style/border_edge.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

void NGFragmentPainter::PaintOutline(const PaintInfo& paint_info,
                                     const LayoutPoint& paint_offset) {
  DCHECK(ShouldPaintSelfOutline(paint_info.phase));

  const ComputedStyle& style = paint_fragment_.Style();
  if (!style.HasOutline() || style.Visibility() != EVisibility::kVisible)
    return;

  // Only paint the focus ring by hand if the theme isn't able to draw the focus
  // ring.
  if (style.OutlineStyleIsAuto() &&
      !LayoutTheme::GetTheme().ShouldDrawDefaultFocusRing(
          paint_fragment_.GetNode(), style)) {
    return;
  }

  Vector<LayoutRect> outline_rects;
  paint_fragment_.AddSelfOutlineRect(&outline_rects, paint_offset);
  if (outline_rects.IsEmpty())
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, paint_fragment_, paint_info.phase))
    return;

  DrawingRecorder recorder(paint_info.context, paint_fragment_,
                           paint_info.phase);
  PaintOutlineRects(paint_info, outline_rects, style);
}

void NGFragmentPainter::PaintDescendantOutlines(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  DCHECK(ShouldPaintDescendantOutlines(paint_info.phase));

  NGOutlineUtils::FragmentMap anchor_fragment_map;
  NGOutlineUtils::OutlineRectMap outline_rect_map;

  DCHECK(paint_fragment_.PhysicalFragment().IsBox());
  if (!paint_fragment_.GetLayoutObject()->OutlineMayBeAffectedByDescendants())
    return;
  NGOutlineUtils::CollectDescendantOutlines(
      ToNGPhysicalBoxFragment(paint_fragment_.PhysicalFragment()),
      NGPhysicalOffset(paint_offset.X(), paint_offset.Y()),
      &anchor_fragment_map, &outline_rect_map);

  // Paint all outlines.
  if (anchor_fragment_map.size() == 0)
    return;
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, paint_fragment_,
          PaintPhase::kDescendantOutlinesOnly))
    return;

  DrawingRecorder recorder(paint_info.context, paint_fragment_,
                           PaintPhase::kDescendantOutlinesOnly);
  for (auto& anchor_iter : anchor_fragment_map) {
    const NGPhysicalFragment* fragment = anchor_iter.value;
    Vector<LayoutRect>* outline_rects =
        &outline_rect_map.find(anchor_iter.key)->value;
    PaintOutlineRects(paint_info, *outline_rects, fragment->Style());
  }
}

void NGFragmentPainter::AddPDFURLRectIfNeeded(const PaintInfo& paint_info,
                                              const LayoutPoint& paint_offset) {
  DCHECK(paint_info.IsPrinting());

  // TODO(layout-dev): Should use break token when NG has its own tree building.
  if (paint_fragment_.GetLayoutObject()->IsElementContinuation() ||
      !paint_fragment_.GetNode() || !paint_fragment_.GetNode()->IsLink() ||
      paint_fragment_.Style().Visibility() != EVisibility::kVisible)
    return;

  KURL url = ToElement(paint_fragment_.GetNode())->HrefURL();
  if (!url.IsValid())
    return;

  IntRect rect = PixelSnappedIntRect(paint_fragment_.VisualRect());
  if (rect.IsEmpty())
    return;

  const NGPhysicalFragment& fragment = paint_fragment_.PhysicalFragment();
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, paint_fragment_,
          DisplayItem::kPrintedContentPDFURLRect))
    return;

  DrawingRecorder recorder(paint_info.context, paint_fragment_,
                           DisplayItem::kPrintedContentPDFURLRect);

  Document& document = fragment.GetLayoutObject()->GetDocument();
  if (url.HasFragmentIdentifier() &&
      EqualIgnoringFragmentIdentifier(url, document.BaseURL())) {
    String fragment_name = url.FragmentIdentifier();
    if (document.FindAnchor(fragment_name))
      paint_info.context.SetURLFragmentForRect(fragment_name, rect);
    return;
  }
  paint_info.context.SetURLForRect(url, rect);
}

}  // namespace blink
