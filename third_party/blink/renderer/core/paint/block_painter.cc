// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/block_painter.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/editing/drag_caret.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_api_shim.h"
#include "third_party/blink/renderer/core/layout/api/line_layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_flexible_box.h"
#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/adjust_paint_offset_scope.h"
#include "third_party/blink/renderer/core/paint/block_flow_painter.h"
#include "third_party/blink/renderer/core/paint/box_clipper.h"
#include "third_party/blink/renderer/core/paint/box_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/scroll_recorder.h"
#include "third_party/blink/renderer/core/paint/scrollable_area_painter.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/scroll_hit_test_display_item.h"

namespace blink {

DISABLE_CFI_PERF
void BlockPainter::Paint(const PaintInfo& paint_info,
                         const LayoutPoint& paint_offset) {
  AdjustPaintOffsetScope adjustment(layout_block_, paint_info, paint_offset);
  auto adjusted_paint_offset = adjustment.AdjustedPaintOffset();
  auto& local_paint_info = adjustment.MutablePaintInfo();

  if (!IntersectsPaintRect(local_paint_info, adjusted_paint_offset))
    return;

  PaintPhase original_phase = local_paint_info.phase;

  // There are some cases where not all clipped visual overflow is accounted
  // for.
  // FIXME: reduce the number of such cases.
  ContentsClipBehavior contents_clip_behavior = kForceContentsClip;
  if (layout_block_.ShouldClipOverflow() && !layout_block_.HasControlClip() &&
      !layout_block_.ShouldPaintCarets())
    contents_clip_behavior = kSkipContentsClipIfPossible;

  if (original_phase == PaintPhase::kOutline) {
    local_paint_info.phase = PaintPhase::kDescendantOutlinesOnly;
  } else if (ShouldPaintSelfBlockBackground(original_phase)) {
    local_paint_info.phase = PaintPhase::kSelfBlockBackgroundOnly;
    layout_block_.PaintObject(local_paint_info, adjusted_paint_offset);
    if (ShouldPaintDescendantBlockBackgrounds(original_phase))
      local_paint_info.phase = PaintPhase::kDescendantBlockBackgroundsOnly;
  }

  if (original_phase != PaintPhase::kSelfBlockBackgroundOnly &&
      original_phase != PaintPhase::kSelfOutlineOnly) {
    base::Optional<BoxClipper> clipper;
    // We have already applied clip in SVGForeignObjectClipper.
    if (!layout_block_.IsSVGForeignObject() ||
        RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      clipper.emplace(layout_block_, local_paint_info, adjusted_paint_offset,
                      contents_clip_behavior);
    }
    layout_block_.PaintObject(local_paint_info, adjusted_paint_offset);
  }

  if (ShouldPaintSelfOutline(original_phase)) {
    local_paint_info.phase = PaintPhase::kSelfOutlineOnly;
    layout_block_.PaintObject(local_paint_info, adjusted_paint_offset);
  }

  // Our scrollbar widgets paint exactly when we tell them to, so that they work
  // properly with z-index. We paint after we painted the background/border, so
  // that the scrollbars will sit above the background/border.
  local_paint_info.phase = original_phase;
  PaintOverflowControlsIfNeeded(local_paint_info, adjusted_paint_offset);
}

void BlockPainter::PaintOverflowControlsIfNeeded(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  if (layout_block_.HasOverflowClip() &&
      layout_block_.Style()->Visibility() == EVisibility::kVisible &&
      ShouldPaintSelfBlockBackground(paint_info.phase)) {
    base::Optional<ClipRecorder> clip_recorder;
    if (!layout_block_.Layer()->IsSelfPaintingLayer()) {
      LayoutRect clip_rect = layout_block_.BorderBoxRect();
      clip_rect.MoveBy(paint_offset);
      clip_recorder.emplace(paint_info.context, layout_block_,
                            DisplayItem::kClipScrollbarsToBoxBounds,
                            PixelSnappedIntRect(clip_rect));
    }
    ScrollableAreaPainter(*layout_block_.Layer()->GetScrollableArea())
        .PaintOverflowControls(paint_info, RoundedIntPoint(paint_offset),
                               false /* painting_overlay_controls */);
  }
}

void BlockPainter::PaintChildren(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  for (LayoutBox* child = layout_block_.FirstChildBox(); child;
       child = child->NextSiblingBox())
    PaintChild(*child, paint_info, paint_offset);
}

void BlockPainter::PaintChild(const LayoutBox& child,
                              const PaintInfo& paint_info,
                              const LayoutPoint& paint_offset) {
  LayoutPoint child_point =
      layout_block_.FlipForWritingModeForChildForPaint(&child, paint_offset);
  if (!child.HasSelfPaintingLayer() && !child.IsFloating() &&
      !child.IsColumnSpanAll())
    child.Paint(paint_info, child_point);
}

void BlockPainter::PaintChildrenOfFlexibleBox(
    const LayoutFlexibleBox& layout_flexible_box,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (const LayoutBox* child = layout_flexible_box.GetOrderIterator().First();
       child; child = layout_flexible_box.GetOrderIterator().Next())
    BlockPainter(layout_flexible_box)
        .PaintAllChildPhasesAtomically(*child, paint_info, paint_offset);
}

void BlockPainter::PaintAllChildPhasesAtomically(
    const LayoutBox& child,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutPoint child_point =
      layout_block_.FlipForWritingModeForChildForPaint(&child, paint_offset);
  if (!child.HasSelfPaintingLayer() && !child.IsFloating())
    ObjectPainter(child).PaintAllPhasesAtomically(paint_info, child_point);
}

void BlockPainter::PaintInlineBox(const InlineBox& inline_box,
                                  const PaintInfo& paint_info,
                                  const LayoutPoint& paint_offset) {
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection)
    return;

  // Text clips are painted only for the direct inline children of the object
  // that has a text clip style on it, not block children.
  DCHECK(paint_info.phase != PaintPhase::kTextClip);

  LayoutPoint child_point = paint_offset;
  if (inline_box.Parent()
          ->GetLineLayoutItem()
          .Style()
          ->IsFlippedBlocksWritingMode()) {
    // Faster than calling containingBlock().
    child_point =
        LineLayoutAPIShim::LayoutObjectFrom(inline_box.GetLineLayoutItem())
            ->ContainingBlock()
            ->FlipForWritingModeForChildForPaint(
                ToLayoutBox(LineLayoutAPIShim::LayoutObjectFrom(
                    inline_box.GetLineLayoutItem())),
                child_point);
  }

  ObjectPainter(
      *LineLayoutAPIShim::ConstLayoutObjectFrom(inline_box.GetLineLayoutItem()))
      .PaintAllPhasesAtomically(paint_info, child_point);
}

void BlockPainter::PaintScrollHitTestDisplayItem(const PaintInfo& paint_info) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled());

  // Scroll hit test display items are only needed for compositing. This flag is
  // used for for printing and drag images which do not need hit testing.
  if (paint_info.GetGlobalPaintFlags() & kGlobalPaintFlattenCompositingLayers)
    return;

  // The scroll hit test layer is in the unscrolled and unclipped space so the
  // scroll hit test layer can be enlarged beyond the clip. This will let us fix
  // crbug.com/753124 in the future where the scrolling element's border is hit
  // test differently if composited.

  const auto* fragment = paint_info.FragmentToPaint(layout_block_);
  const auto* properties = fragment ? fragment->PaintProperties() : nullptr;

  // If there is an associated scroll node, emit a scroll hit test display item.
  if (properties && properties->Scroll()) {
    DCHECK(properties->ScrollTranslation());
    // The local border box properties are used instead of the contents
    // properties so that the scroll hit test is not clipped or scrolled.
    ScopedPaintChunkProperties scroll_hit_test_properties(
        paint_info.context.GetPaintController(),
        fragment->LocalBorderBoxProperties(), layout_block_,
        DisplayItem::kScrollHitTest);
    ScrollHitTestDisplayItem::Record(paint_info.context, layout_block_,
                                     DisplayItem::kScrollHitTest,
                                     properties->ScrollTranslation());
  }
}

DISABLE_CFI_PERF
void BlockPainter::PaintObject(const PaintInfo& paint_info,
                               const LayoutPoint& paint_offset) {
  if (layout_block_.IsTruncated())
    return;

  const PaintPhase paint_phase = paint_info.phase;

  if (ShouldPaintSelfBlockBackground(paint_phase)) {
    if (layout_block_.Style()->Visibility() == EVisibility::kVisible &&
        layout_block_.HasBoxDecorationBackground())
      layout_block_.PaintBoxDecorationBackground(paint_info, paint_offset);
    // Record the scroll hit test after the background so background squashing
    // is not affected. Hit test order would be equivalent if this were
    // immediately before the background.
    if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
      PaintScrollHitTestDisplayItem(paint_info);
    // We're done. We don't bother painting any children.
    if (paint_phase == PaintPhase::kSelfBlockBackgroundOnly)
      return;
  }

  if (paint_phase == PaintPhase::kMask &&
      layout_block_.Style()->Visibility() == EVisibility::kVisible) {
    layout_block_.PaintMask(paint_info, paint_offset);
    return;
  }

  if (paint_phase == PaintPhase::kClippingMask &&
      layout_block_.Style()->Visibility() == EVisibility::kVisible) {
    // SPv175 always paints clipping mask in PaintLayerPainter.
    DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
    BoxPainter(layout_block_).PaintClippingMask(paint_info, paint_offset);
    return;
  }

  if (paint_phase == PaintPhase::kForeground && paint_info.IsPrinting())
    ObjectPainter(layout_block_)
        .AddPDFURLRectIfNeeded(paint_info, paint_offset);

  if (paint_phase != PaintPhase::kSelfOutlineOnly) {
    base::Optional<ScopedPaintChunkProperties> scoped_scroll_property;
    base::Optional<ScrollRecorder> scroll_recorder;
    base::Optional<PaintInfo> scrolled_paint_info;
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      if (const auto* fragment = paint_info.FragmentToPaint(layout_block_)) {
        const auto* object_properties = fragment->PaintProperties();
        auto* scroll_translation = object_properties
                                       ? object_properties->ScrollTranslation()
                                       : nullptr;
        if (scroll_translation) {
          scoped_scroll_property.emplace(
              paint_info.context.GetPaintController(), scroll_translation,
              layout_block_, DisplayItem::PaintPhaseToScrollType(paint_phase));
          scrolled_paint_info.emplace(paint_info);
          if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
            scrolled_paint_info->UpdateCullRectForScrollingContents(
                EnclosingIntRect(layout_block_.OverflowClipRect(paint_offset)),
                scroll_translation->Matrix().ToAffineTransform());
          } else {
            scrolled_paint_info->UpdateCullRect(
                scroll_translation->Matrix().ToAffineTransform());
          }
        }
      }
    } else if (layout_block_.HasOverflowClip()) {
      IntSize scroll_offset = layout_block_.ScrolledContentOffset();
      if (layout_block_.Layer()->ScrollsOverflow() || !scroll_offset.IsZero()) {
        scroll_recorder.emplace(paint_info.context, layout_block_, paint_phase,
                                scroll_offset);
        scrolled_paint_info.emplace(paint_info);
        AffineTransform transform;
        transform.Translate(-scroll_offset.Width(), -scroll_offset.Height());
        scrolled_paint_info->UpdateCullRect(transform);
      }
    }

    const PaintInfo& contents_paint_info =
        scrolled_paint_info ? *scrolled_paint_info : paint_info;

    if (layout_block_.IsLayoutBlockFlow()) {
      BlockFlowPainter block_flow_painter(ToLayoutBlockFlow(layout_block_));
      block_flow_painter.PaintContents(contents_paint_info, paint_offset);
      if (paint_phase == PaintPhase::kFloat ||
          paint_phase == PaintPhase::kSelection ||
          paint_phase == PaintPhase::kTextClip)
        block_flow_painter.PaintFloats(contents_paint_info, paint_offset);
    } else {
      PaintContents(contents_paint_info, paint_offset);
    }
  }

  if (ShouldPaintSelfOutline(paint_phase))
    ObjectPainter(layout_block_).PaintOutline(paint_info, paint_offset);

  // If the caret's node's layout object's containing block is this block, and
  // the paint action is PaintPhaseForeground, then paint the caret.
  if (paint_phase == PaintPhase::kForeground &&
      layout_block_.ShouldPaintCarets())
    PaintCarets(paint_info, paint_offset);
}

void BlockPainter::PaintCarets(const PaintInfo& paint_info,
                               const LayoutPoint& paint_offset) {
  LocalFrame* frame = layout_block_.GetFrame();

  if (layout_block_.ShouldPaintCursorCaret())
    frame->Selection().PaintCaret(paint_info.context, paint_offset);

  if (layout_block_.ShouldPaintDragCaret()) {
    frame->GetPage()->GetDragCaret().PaintDragCaret(frame, paint_info.context,
                                                    paint_offset);
  }
}

DISABLE_CFI_PERF
bool BlockPainter::IntersectsPaintRect(
    const PaintInfo& paint_info,
    const LayoutPoint& adjusted_paint_offset) const {
  LayoutRect overflow_rect;
  if (paint_info.IsPrinting() && layout_block_.IsAnonymousBlock() &&
      layout_block_.ChildrenInline()) {
    // For case <a href="..."><div>...</div></a>, when m_layoutBlock is the
    // anonymous container of <a>, the anonymous container's visual overflow is
    // empty, but we need to continue painting to output <a>'s PDF URL rect
    // which covers the continuations, as if we included <a>'s PDF URL rect into
    // m_layoutBlock's visual overflow.
    Vector<LayoutRect> rects;
    layout_block_.AddElementVisualOverflowRects(rects, LayoutPoint());
    overflow_rect = UnionRect(rects);
  }
  overflow_rect.Unite(layout_block_.VisualOverflowRect());

  bool uses_composited_scrolling = layout_block_.HasOverflowModel() &&
                                   layout_block_.UsesCompositedScrolling();

  if (uses_composited_scrolling) {
    LayoutRect layout_overflow_rect = layout_block_.LayoutOverflowRect();
    overflow_rect.Unite(layout_overflow_rect);
  }
  layout_block_.FlipForWritingMode(overflow_rect);

  // Scrolling is applied in physical space, which is why it is after the flip
  // above.
  if (uses_composited_scrolling) {
    overflow_rect.Move(-layout_block_.ScrolledContentOffset());
  }

  overflow_rect.MoveBy(adjusted_paint_offset);
  return paint_info.GetCullRect().IntersectsCullRect(overflow_rect);
}

void BlockPainter::PaintContents(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  DCHECK(!layout_block_.ChildrenInline());
  PaintInfo paint_info_for_descendants = paint_info.ForDescendants();
  layout_block_.PaintChildren(paint_info_for_descendants, paint_offset);
}

}  // namespace blink
