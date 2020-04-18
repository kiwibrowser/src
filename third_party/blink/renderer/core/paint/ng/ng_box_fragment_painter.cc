// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/ng/ng_box_fragment_painter.h"

#include "third_party/blink/renderer/core/layout/background_bleed_avoidance.h"
#include "third_party/blink/renderer/core/layout/hit_test_location.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_table.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_border_edges.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_box_strut.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_mixin.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/paint/adjust_paint_offset_scope.h"
#include "third_party/blink/renderer/core/paint/background_image_geometry.h"
#include "third_party/blink/renderer/core/paint/box_decoration_data.h"
#include "third_party/blink/renderer/core/paint/ng/ng_box_clipper.h"
#include "third_party/blink/renderer/core/paint/ng/ng_fragment_painter.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_text_fragment_painter.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/core/paint/scroll_recorder.h"
#include "third_party/blink/renderer/core/paint/scrollable_area_painter.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect_outsets.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"

namespace blink {

namespace {

LayoutRectOutsets BoxStrutToLayoutRectOutsets(
    const NGPixelSnappedPhysicalBoxStrut& box_strut) {
  return LayoutRectOutsets(
      LayoutUnit(box_strut.top), LayoutUnit(box_strut.right),
      LayoutUnit(box_strut.bottom), LayoutUnit(box_strut.left));
}

bool ShouldPaintBoxFragmentBorders(const LayoutObject& object) {
  if (!object.IsTableCell())
    return true;
  // Collapsed borders are painted by the containing table, not by each
  // individual table cell.
  return !ToLayoutTableCell(object).Table()->ShouldCollapseBorders();
}

}  // anonymous namespace

NGBoxFragmentPainter::NGBoxFragmentPainter(const NGPaintFragment& box)
    : BoxPainterBase(
          box,
          &box.GetLayoutObject()->GetDocument(),
          box.Style(),
          box.GetLayoutObject()->GeneratingNode(),
          BoxStrutToLayoutRectOutsets(box.PhysicalFragment().BorderWidths()),
          BoxStrutToLayoutRectOutsets(
              ToNGPhysicalBoxFragment(box.PhysicalFragment()).Padding()),
          box.PhysicalFragment().Layer()),
      box_fragment_(box),
      border_edges_(
          NGBorderEdges::FromPhysical(box.PhysicalFragment().BorderEdges(),
                                      box.Style().GetWritingMode())) {
  DCHECK(box.PhysicalFragment().IsBox());
}

void NGBoxFragmentPainter::Paint(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  const LayoutObject* layout_object = PhysicalFragment().GetLayoutObject();
  DCHECK(layout_object && layout_object->IsBox());
  if (!PhysicalFragment().IsPlacedByLayoutNG()) {
    // |fragment.Offset()| is valid only when it is placed by LayoutNG parent.
    // Use LayoutBox::Location() if not. crbug.com/788590
    AdjustPaintOffsetScope adjustment(ToLayoutBox(*layout_object), paint_info,
                                      paint_offset);
    PaintWithAdjustedOffset(adjustment.MutablePaintInfo(),
                            adjustment.AdjustedPaintOffset());
    return;
  }
  DCHECK_EQ(layout_object->Style(PhysicalFragment().UsesFirstLineStyle()),
            &PhysicalFragment().Style());
  AdjustPaintOffsetScope adjustment(box_fragment_, paint_info, paint_offset);
  PaintWithAdjustedOffset(adjustment.MutablePaintInfo(),
                          adjustment.AdjustedPaintOffset());
}

void NGBoxFragmentPainter::PaintInlineBox(const PaintInfo& paint_info,
                                          const LayoutPoint& paint_offset) {
  const LayoutPoint adjusted_paint_offset =
      paint_offset + box_fragment_.Offset().ToLayoutPoint();
  if (paint_info.phase == PaintPhase::kForeground)
    PaintBoxDecorationBackground(paint_info, adjusted_paint_offset);

  PaintObject(paint_info, adjusted_paint_offset, true);
}

void NGBoxFragmentPainter::PaintWithAdjustedOffset(
    PaintInfo& info,
    const LayoutPoint& paint_offset) {
  if (!IntersectsPaintRect(info, paint_offset))
    return;

  if (PhysicalFragment().IsAtomicInline())
    return PaintAtomicInline(info, paint_offset);

  PaintPhase original_phase = info.phase;

  if (original_phase == PaintPhase::kOutline) {
    info.phase = PaintPhase::kDescendantOutlinesOnly;
  } else if (ShouldPaintSelfBlockBackground(original_phase)) {
    info.phase = PaintPhase::kSelfBlockBackgroundOnly;
    PaintObject(info, paint_offset);
    if (ShouldPaintDescendantBlockBackgrounds(original_phase))
      info.phase = PaintPhase::kDescendantBlockBackgroundsOnly;
  }

  if (original_phase != PaintPhase::kSelfBlockBackgroundOnly &&
      original_phase != PaintPhase::kSelfOutlineOnly) {
    NGBoxClipper box_clipper(box_fragment_, info);
    PaintObject(info, paint_offset);
  }

  if (ShouldPaintSelfOutline(original_phase)) {
    info.phase = PaintPhase::kSelfOutlineOnly;
    PaintObject(info, paint_offset);
  }

  // Our scrollbar widgets paint exactly when we tell them to, so that they work
  // properly with z-index. We paint after we painted the background/border, so
  // that the scrollbars will sit above the background/border.
  info.phase = original_phase;
  PaintOverflowControlsIfNeeded(info, paint_offset);
}

void NGBoxFragmentPainter::PaintObject(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    bool suppress_box_decoration_background) {
  const PaintPhase paint_phase = paint_info.phase;
  const ComputedStyle& style = box_fragment_.Style();
  bool is_visible = style.Visibility() == EVisibility::kVisible;

  if (ShouldPaintSelfBlockBackground(paint_phase)) {
    // TODO(eae): style.HasBoxDecorationBackground isn't good enough, it needs
    // to check the object as some objects may have box decoration background
    // other than from their own style.
    // TODO(eae): This should not be needed both here and in PaintInlineBox.
    if (!suppress_box_decoration_background && is_visible &&
        style.HasBoxDecorationBackground())
      PaintBoxDecorationBackground(paint_info, paint_offset);

    // Record the scroll hit test after the background so background squashing
    // is not affected. Hit test order would be equivalent if this were
    // immediately before the background.
    // if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    //  PaintScrollHitTestDisplayItem(paint_info);

    // We're done. We don't bother painting any children.
    if (paint_phase == PaintPhase::kSelfBlockBackgroundOnly)
      return;
  }

  if (paint_phase == PaintPhase::kMask && is_visible)
    return PaintMask(paint_info, paint_offset);

  if (paint_phase == PaintPhase::kClippingMask && is_visible)
    return PaintClippingMask(paint_info, paint_offset);

  if (paint_phase == PaintPhase::kForeground && paint_info.IsPrinting()) {
    NGFragmentPainter(box_fragment_)
        .AddPDFURLRectIfNeeded(paint_info, paint_offset);
  }

  if (paint_phase != PaintPhase::kSelfOutlineOnly) {
    // TODO(layout-dev): Figure out where paint properties should live.
    const auto& layout_object = *box_fragment_.GetLayoutObject();
    base::Optional<ScopedPaintChunkProperties> scoped_scroll_property;
    base::Optional<PaintInfo> scrolled_paint_info;
    if (const auto* fragment = paint_info.FragmentToPaint(layout_object)) {
      DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
      const auto* object_properties = fragment->PaintProperties();
      auto* scroll_translation =
          object_properties ? object_properties->ScrollTranslation() : nullptr;
      if (scroll_translation) {
        scoped_scroll_property.emplace(
            paint_info.context.GetPaintController(), scroll_translation,
            box_fragment_, DisplayItem::PaintPhaseToScrollType(paint_phase));
        scrolled_paint_info.emplace(paint_info);
        if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
          scrolled_paint_info->UpdateCullRectForScrollingContents(
              EnclosingIntRect(PhysicalFragment().OverflowClipRect(
                  paint_offset, kIgnorePlatformOverlayScrollbarSize)),
              scroll_translation->Matrix().ToAffineTransform());
        } else {
          scrolled_paint_info->UpdateCullRect(
              scroll_translation->Matrix().ToAffineTransform());
        }
      }
    }

    const PaintInfo& contents_paint_info =
        scrolled_paint_info ? *scrolled_paint_info : paint_info;

    if (PhysicalFragment().ChildrenInline()) {
      if (PhysicalFragment().IsBlockFlow()) {
        PaintBlockFlowContents(contents_paint_info, paint_offset);
        if (paint_phase == PaintPhase::kFloat ||
            paint_phase == PaintPhase::kSelection ||
            paint_phase == PaintPhase::kTextClip)
          PaintFloats(contents_paint_info, paint_offset);
      } else {
        PaintInlineChildren(box_fragment_.Children(), contents_paint_info,
                            paint_offset);
      }
    } else {
      PaintBlockChildren(contents_paint_info, paint_offset);
    }
  }

  if (ShouldPaintSelfOutline(paint_phase))
    NGFragmentPainter(box_fragment_).PaintOutline(paint_info, paint_offset);
  if (ShouldPaintDescendantOutlines(paint_phase))
    NGFragmentPainter(box_fragment_)
        .PaintDescendantOutlines(paint_info, paint_offset);
  // TODO(layout-dev): Implement once we have selections in LayoutNG.
  // If the caret's node's layout object's containing block is this block, and
  // the paint action is PaintPhaseForeground, then paint the caret.
  // if (paint_phase == PaintPhase::kForeground &&
  //     box_fragment_.ShouldPaintCarets())
  //  PaintCarets(paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintBlockFlowContents(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  // Avoid painting descendants of the root element when stylesheets haven't
  // loaded. This eliminates FOUC.  It's ok not to draw, because later on, when
  // all the stylesheets do load, styleResolverMayHaveChanged() on Document will
  // trigger a full paint invalidation.
  // TODO(layout-dev): Handle without delegating to LayoutObject.
  LayoutObject* layout_object = box_fragment_.GetLayoutObject();
  if (layout_object->GetDocument().DidLayoutWithPendingStylesheets() &&
      !layout_object->IsLayoutView()) {
    return;
  }

  DCHECK(PhysicalFragment().ChildrenInline());

  LayoutRect overflow_rect(box_fragment_.VisualOverflowRect());
  overflow_rect.MoveBy(paint_offset);
  if (!paint_info.GetCullRect().IntersectsCullRect(overflow_rect))
    return;

  if (paint_info.phase == PaintPhase::kMask) {
    if (DrawingRecorder::UseCachedDrawingIfPossible(
            paint_info.context, box_fragment_,
            DisplayItem::PaintPhaseToDrawingType(paint_info.phase)))
      return;
    DrawingRecorder recorder(
        paint_info.context, box_fragment_,
        DisplayItem::PaintPhaseToDrawingType(paint_info.phase));
    PaintMask(paint_info, paint_offset);
    return;
  }

  const auto& children = box_fragment_.Children();
  if (ShouldPaintDescendantOutlines(paint_info.phase))
    PaintInlineChildrenOutlines(children, paint_info, paint_offset);
  else
    PaintLineBoxChildren(children, paint_info.ForDescendants(), paint_offset);
}

void NGBoxFragmentPainter::PaintInlineChild(const NGPaintFragment& child,
                                            const PaintInfo& paint_info,
                                            const LayoutPoint& paint_offset) {
  // Atomic-inline children should be painted by PaintAtomicInlineChild.
  DCHECK(!child.PhysicalFragment().IsAtomicInline());

  const NGPhysicalFragment& fragment = child.PhysicalFragment();
  PaintInfo descendants_info = paint_info.ForDescendants();
  if (fragment.Type() == NGPhysicalFragment::kFragmentText) {
    PaintTextChild(child, descendants_info, paint_offset);
  } else if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
    if (child.HasSelfPaintingLayer())
      return;
    NGBoxFragmentPainter(child).PaintInlineBox(descendants_info, paint_offset);
  } else {
    NOTREACHED();
  }
}

namespace {
bool FragmentRequiresLegacyFallback(const NGPhysicalFragment& fragment) {
  // Fallback to LayoutObject if this is a root of NG block layout.
  // If this box is for this painter, LayoutNGBlockFlow will call back.
  if (fragment.IsBlockLayoutRoot())
    return true;

  // TODO(kojii): Review if this is still needed.
  LayoutObject* layout_object = fragment.GetLayoutObject();
  return layout_object->IsLayoutReplaced();
}
}  // anonymous namespace

void NGBoxFragmentPainter::PaintBlockChildren(const PaintInfo& paint_info,
                                              const LayoutPoint& paint_offset) {
  for (const auto& child : box_fragment_.Children()) {
    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    if (child->HasSelfPaintingLayer() || fragment.IsFloating())
      continue;

    if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
      if (FragmentRequiresLegacyFallback(fragment))
        fragment.GetLayoutObject()->Paint(paint_info, paint_offset);
      else
        NGBoxFragmentPainter(*child).Paint(paint_info, paint_offset);
    } else {
      NOTREACHED() << fragment.ToString();
    }
  }
}

void NGBoxFragmentPainter::PaintFloatingChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (const auto& child : children) {
    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    if (child->HasSelfPaintingLayer())
      continue;
    if (fragment.IsFloating()) {
      // TODO(kojii): The float is outside of the inline formatting context and
      // that it maybe another NG inline formatting context, NG block layout, or
      // legacy. NGBoxFragmentPainter can handle only the first case. In order
      // to cover more tests for other two cases, we always fallback to legacy,
      // which will forward back to NGBoxFragmentPainter if the float is for
      // NGBoxFragmentPainter. We can shortcut this for the first case when
      // we're more stable.
      ObjectPainter(*child->GetLayoutObject())
          .PaintAllPhasesAtomically(paint_info, paint_offset);
    } else {
      PaintFloatingChildren(child->Children(), paint_info, paint_offset);
    }
  }
}

void NGBoxFragmentPainter::PaintFloats(const PaintInfo& paint_info,
                                       const LayoutPoint& paint_offset) {
  // TODO(eae): The legacy paint code currently handles most floats, if they can
  // be painted by PaintNG BlockFlowPainter::PaintFloats will then call
  // NGBlockFlowPainter::Paint on each float.
  // This code is currently only used for floats within a block within inline
  // children.
  PaintInfo float_paint_info(paint_info);
  if (paint_info.phase == PaintPhase::kFloat)
    float_paint_info.phase = PaintPhase::kForeground;
  PaintFloatingChildren(box_fragment_.Children(), float_paint_info,
                        paint_offset);
}

void NGBoxFragmentPainter::PaintMask(const PaintInfo& paint_info,
                                     const LayoutPoint& paint_offset) {
  DCHECK_EQ(PaintPhase::kMask, paint_info.phase);
  const ComputedStyle& style = box_fragment_.Style();
  if (!style.HasMask() || style.Visibility() != EVisibility::kVisible)
    return;

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, box_fragment_, paint_info.phase))
    return;

  // TODO(eae): Switch to LayoutNG version of BackgroundImageGeometry.
  BackgroundImageGeometry geometry(*static_cast<const LayoutBoxModelObject*>(
      box_fragment_.GetLayoutObject()));

  DrawingRecorder recorder(paint_info.context, box_fragment_, paint_info.phase);
  LayoutRect paint_rect =
      LayoutRect(paint_offset, box_fragment_.Size().ToLayoutSize());
  PaintMaskImages(paint_info, paint_rect, box_fragment_, geometry,
                  border_edges_.line_left, border_edges_.line_right);
}

void NGBoxFragmentPainter::PaintClippingMask(const PaintInfo&,
                                             const LayoutPoint&) {
  // TODO(eae): Implement.
}

void NGBoxFragmentPainter::PaintBoxDecorationBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutRect paint_rect;
  if (!IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          box_fragment_, paint_info)) {
    // TODO(eae): We need better converters for ng geometry types. Long term we
    // probably want to change the paint code to take NGPhysical* but that is a
    // much bigger change.
    NGPhysicalSize size = box_fragment_.Size();
    paint_rect = LayoutRect(LayoutPoint(), LayoutSize(size.width, size.height));
  }

  paint_rect.MoveBy(paint_offset);

  bool painting_overflow_contents =
      IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          box_fragment_, paint_info);
  const ComputedStyle& style = box_fragment_.Style();

  // TODO(layout-dev): Implement support for painting overflow contents.
  const DisplayItemClient& display_item_client = box_fragment_;
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, display_item_client,
          DisplayItem::kBoxDecorationBackground))
    return;

  DrawingRecorder recorder(paint_info.context, display_item_client,
                           DisplayItem::kBoxDecorationBackground);
  BoxDecorationData box_decoration_data(PhysicalFragment());
  GraphicsContextStateSaver state_saver(paint_info.context, false);

  if (!painting_overflow_contents) {
    PaintNormalBoxShadow(paint_info, paint_rect, style, border_edges_.line_left,
                         border_edges_.line_right);

    if (BleedAvoidanceIsClipping(box_decoration_data.bleed_avoidance)) {
      state_saver.Save();
      FloatRoundedRect border = style.GetRoundedBorderFor(
          paint_rect, border_edges_.line_left, border_edges_.line_right);
      paint_info.context.ClipRoundedRect(border);

      if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
        paint_info.context.BeginLayer();
    }
  }

  // TODO(layout-dev): Support theme painting.
  bool theme_painted = false;

  bool should_paint_background =
      !theme_painted &&
      (!paint_info.SkipRootBackground() ||
       paint_info.PaintContainer() != box_fragment_.GetLayoutObject());
  if (should_paint_background) {
    PaintBackground(paint_info, paint_rect,
                    box_decoration_data.background_color,
                    box_decoration_data.bleed_avoidance);
  }

  if (!painting_overflow_contents) {
    PaintInsetBoxShadowWithBorderRect(paint_info, paint_rect, style,
                                      border_edges_.line_left,
                                      border_edges_.line_right);

    if (box_decoration_data.has_border_decoration &&
        ShouldPaintBoxFragmentBorders(*box_fragment_.GetLayoutObject())) {
      Node* generating_node = box_fragment_.GetLayoutObject()->GeneratingNode();
      const Document& document = box_fragment_.GetLayoutObject()->GetDocument();
      PaintBorder(box_fragment_, document, generating_node, paint_info,
                  paint_rect, style, box_decoration_data.bleed_avoidance,
                  border_edges_.line_left, border_edges_.line_right);
    }
  }

  if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
    paint_info.context.EndLayer();
}

void NGBoxFragmentPainter::PaintInlineChildBoxUsingLegacyFallback(
    const NGPhysicalFragment& fragment,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutPoint& legacy_paint_offset) {
  LayoutObject* child_layout_object = fragment.GetLayoutObject();
  DCHECK(child_layout_object);
  if (child_layout_object->IsLayoutNGMixin() &&
      ToLayoutBlockFlow(child_layout_object)->PaintFragment()) {
    // This object will use NGBoxFragmentPainter. NGBoxFragmentPainter expects
    // |paint_offset| relative to the parent, even when in inline context.
    LayoutPoint child_point =
        FlipForWritingModeForChild(fragment, paint_offset);
    child_layout_object->Paint(paint_info, child_point);
    return;
  }

  // When in inline context, pre-NG painters expect |paint_offset| of their
  // block container.
  if (child_layout_object->IsAtomicInlineLevel()) {
    // Pre-NG painters also expect callers to use |PaintAllPhasesAtomically()|
    // for atomic inlines.
    LayoutPoint child_point =
        FlipForWritingModeForChild(fragment, legacy_paint_offset);
    ObjectPainter(*child_layout_object)
        .PaintAllPhasesAtomically(paint_info, child_point);
    return;
  }

  child_layout_object->Paint(paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintAllPhasesAtomically(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  // Pass PaintPhaseSelection and PaintPhaseTextClip is handled by the regular
  // foreground paint implementation. We don't need complete painting for these
  // phases.
  PaintPhase phase = paint_info.phase;
  if (phase == PaintPhase::kSelection || phase == PaintPhase::kTextClip)
    return PaintObject(paint_info, paint_offset);

  if (phase != PaintPhase::kForeground)
    return;

  PaintInfo info(paint_info);

  NGBoxClipper box_clipper(box_fragment_, info);

  info.phase = PaintPhase::kBlockBackground;
  PaintObject(info, paint_offset);

  info.phase = PaintPhase::kFloat;
  PaintObject(info, paint_offset);

  info.phase = PaintPhase::kForeground;
  PaintObject(info, paint_offset);

  info.phase = PaintPhase::kOutline;
  PaintObject(info, paint_offset);
}

void NGBoxFragmentPainter::PaintLineBoxChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& line_boxes,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  DCHECK(!ShouldPaintSelfOutline(paint_info.phase) &&
         !ShouldPaintDescendantOutlines(paint_info.phase));

  // Only paint during the foreground/selection phases.
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection &&
      paint_info.phase != PaintPhase::kTextClip &&
      paint_info.phase != PaintPhase::kMask)
    return;

  // The only way an inline could paint like this is if it has a layer.
  const auto* layout_object = box_fragment_.GetLayoutObject();
  DCHECK(layout_object->IsLayoutBlock() ||
         (layout_object->IsLayoutInline() && layout_object->HasLayer()));

  // if (paint_info.phase == PaintPhase::kForeground && paint_info.IsPrinting())
  //  AddPDFURLRectsForInlineChildrenRecursively(layout_object, paint_info,
  //                                             paint_offset);

  // If we have no lines then we have no work to do.
  if (!line_boxes.size())
    return;

  // TODO(layout-dev): Early return if no line intersects cull rect.
  for (const auto& line : line_boxes) {
    if (line->PhysicalFragment().IsFloatingOrOutOfFlowPositioned())
      continue;
    const LayoutPoint child_offset =
        paint_offset + line->Offset().ToLayoutPoint();
    if (line->PhysicalFragment().IsListMarker()) {
      PaintAtomicInlineChild(*line, paint_info, paint_offset, paint_offset);
      continue;
    }
    DCHECK(line->PhysicalFragment().IsLineBox())
        << line->PhysicalFragment().ToString();
    PaintInlineChildren(line->Children(), paint_info, child_offset);
  }
}

void NGBoxFragmentPainter::PaintInlineChildren(
    const Vector<std::unique_ptr<NGPaintFragment>>& inline_children,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (const auto& child : inline_children) {
    if (child->PhysicalFragment().IsFloating())
      continue;
    if (child->PhysicalFragment().IsAtomicInline()) {
      // legacy_paint_offset is local, so we need to remove the offset to
      // lineBox.
      LayoutPoint legacy_paint_offset = paint_offset;
      const NGPaintFragment* parent = child->Parent();
      while (parent && (parent->PhysicalFragment().IsBox() ||
                        parent->PhysicalFragment().IsLineBox())) {
        legacy_paint_offset -= parent->Offset().ToLayoutPoint();
        if (parent->PhysicalFragment().IsLineBox())
          break;
        parent = parent->Parent();
      }

      PaintAtomicInlineChild(*child, paint_info, paint_offset,
                             legacy_paint_offset);
    } else {
      PaintInlineChild(*child, paint_info, paint_offset);
    }
  }
}

void NGBoxFragmentPainter::PaintInlineChildrenOutlines(
    const Vector<std::unique_ptr<NGPaintFragment>>& line_boxes,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  // TODO(layout-dev): Implement.
}

void NGBoxFragmentPainter::PaintAtomicInlineChild(
    const NGPaintFragment& child,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutPoint& legacy_paint_offset) {
  // Inline children should be painted by PaintInlineChild.
  DCHECK(child.PhysicalFragment().IsAtomicInline());

  const NGPhysicalFragment& fragment = child.PhysicalFragment();
  if (child.HasSelfPaintingLayer())
    return;
  if (fragment.Type() == NGPhysicalFragment::kFragmentBox &&
      FragmentRequiresLegacyFallback(fragment)) {
    PaintInlineChildBoxUsingLegacyFallback(fragment, paint_info, paint_offset,
                                           legacy_paint_offset);
  } else {
    NGBoxFragmentPainter(child).PaintAllPhasesAtomically(
        paint_info, paint_offset + fragment.Offset().ToLayoutPoint());
  }
}

void NGBoxFragmentPainter::PaintTextChild(const NGPaintFragment& text_fragment,
                                          const PaintInfo& paint_info,
                                          const LayoutPoint& paint_offset) {
  // Inline blocks should be painted by PaintAtomicInlineChild.
  DCHECK(!text_fragment.PhysicalFragment().IsAtomicInline());

  // The text clip phase already has a DrawingRecorder. Text clips are initiated
  // only in BoxPainterBase::PaintFillLayer, which is already within a
  // DrawingRecorder.
  base::Optional<DrawingRecorder> recorder;
  if (paint_info.phase != PaintPhase::kTextClip) {
    if (DrawingRecorder::UseCachedDrawingIfPossible(
            paint_info.context, text_fragment,
            DisplayItem::PaintPhaseToDrawingType(paint_info.phase)))
      return;
    recorder.emplace(paint_info.context, text_fragment,
                     DisplayItem::PaintPhaseToDrawingType(paint_info.phase));
  }

  NGTextFragmentPainter text_painter(text_fragment);
  text_painter.Paint(paint_info, paint_offset);
}

void NGBoxFragmentPainter::PaintAtomicInline(const PaintInfo& paint_info,
                                             const LayoutPoint& paint_offset) {
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection)
    return;

  // Text clips are painted only for the direct inline children of the object
  // that has a text clip style on it, not block children.
  DCHECK(paint_info.phase != PaintPhase::kTextClip);

  PaintAllPhasesAtomically(paint_info, paint_offset);
}

bool NGBoxFragmentPainter::
    IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
        const NGPaintFragment& fragment,
        const PaintInfo& paint_info) {
  // TODO(layout-dev): Change paint_info.PaintContainer to accept fragments
  // once LayoutNG supports scrolling containers.
  return paint_info.PaintFlags() & kPaintLayerPaintingOverflowContents &&
         !(paint_info.PaintFlags() &
           kPaintLayerPaintingCompositingBackgroundPhase) &&
         box_fragment_.GetLayoutObject() == paint_info.PaintContainer();
}

// Clone of BlockPainter::PaintOverflowControlsIfNeeded
void NGBoxFragmentPainter::PaintOverflowControlsIfNeeded(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  if (box_fragment_.HasOverflowClip() &&
      box_fragment_.Style().Visibility() == EVisibility::kVisible &&
      ShouldPaintSelfBlockBackground(paint_info.phase)) {
    const NGPhysicalBoxFragment& fragment = PhysicalFragment();
    base::Optional<ClipRecorder> clip_recorder;
    if (!fragment.Layer()->IsSelfPaintingLayer()) {
      LayoutRect clip_rect =
          LayoutRect(LayoutPoint(), fragment.Size().ToLayoutSize());
      clip_rect.MoveBy(paint_offset);
      clip_recorder.emplace(paint_info.context, box_fragment_,
                            DisplayItem::kClipScrollbarsToBoxBounds,
                            PixelSnappedIntRect(clip_rect));
    }
    ScrollableAreaPainter(*fragment.Layer()->GetScrollableArea())
        .PaintOverflowControls(paint_info, RoundedIntPoint(paint_offset),
                               false /* painting_overlay_controls */);
  }
}

bool NGBoxFragmentPainter::IntersectsPaintRect(
    const PaintInfo& paint_info,
    const LayoutPoint& adjusted_paint_offset) const {
  // TODO(layout-dev): Add support for scrolling, see
  // BlockPainter::IntersectsPaintRect.
  LayoutRect overflow_rect(box_fragment_.VisualOverflowRect());
  overflow_rect.MoveBy(adjusted_paint_offset);
  return paint_info.GetCullRect().IntersectsCullRect(overflow_rect);
}

void NGBoxFragmentPainter::PaintTextClipMask(GraphicsContext& context,
                                             const IntRect& mask_rect,
                                             const LayoutPoint& paint_offset) {
  PaintInfo paint_info(context, mask_rect, PaintPhase::kTextClip,
                       kGlobalPaintNormalPhase, 0);
  const LayoutSize local_offset = box_fragment_.Offset().ToLayoutSize();
  if (PhysicalFragment().IsBlockFlow()) {
    // TODO(layout-dev): Add support for box-decoration-break: slice
    // See BoxModelObjectPainter::LogicalOffsetOnLine
    // if (box_fragment_.Style().BoxDecorationBreak() ==
    //    EBoxDecorationBreak::kSlice) {
    //  local_offset -= LogicalOffsetOnLine(*flow_box_);
    //}
    PaintBlockFlowContents(paint_info, paint_offset - local_offset);
  } else {
    PaintObject(paint_info, paint_offset - local_offset);
  }
}

LayoutRect NGBoxFragmentPainter::AdjustForScrolledContent(
    const PaintInfo& paint_info,
    const BoxPainterBase::FillLayerInfo& info,
    const LayoutRect& rect) {
  LayoutRect scrolled_paint_rect = rect;
  GraphicsContext& context = paint_info.context;
  const NGPhysicalBoxFragment& physical = PhysicalFragment();

  // Clip to the overflow area.
  if (info.is_clipped_with_local_scrolling &&
      !IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          box_fragment_, paint_info)) {
    context.Clip(FloatRect(physical.OverflowClipRect(rect.Location())));

    // Adjust the paint rect to reflect a scrolled content box with borders at
    // the ends.
    IntSize offset = physical.ScrolledContentOffset();
    scrolled_paint_rect.Move(-offset);
    LayoutRectOutsets borders = BorderOutsets(info);
    scrolled_paint_rect.SetSize(physical.ScrollSize() + borders.Size());
  }
  return scrolled_paint_rect;
}

BoxPainterBase::FillLayerInfo NGBoxFragmentPainter::GetFillLayerInfo(
    const Color& color,
    const FillLayer& bg_layer,
    BackgroundBleedAvoidance bleed_avoidance) const {
  return BoxPainterBase::FillLayerInfo(
      box_fragment_.GetLayoutObject()->GetDocument(), box_fragment_.Style(),
      box_fragment_.HasOverflowClip(), color, bg_layer, bleed_avoidance,
      border_edges_.line_left, border_edges_.line_right);
}

void NGBoxFragmentPainter::PaintBackground(
    const PaintInfo& paint_info,
    const LayoutRect& paint_rect,
    const Color& background_color,
    BackgroundBleedAvoidance bleed_avoidance) {
  // TODO(eae): Switch to LayoutNG version of BackgroundImageGeometry.
  BackgroundImageGeometry geometry(*static_cast<const LayoutBoxModelObject*>(
      box_fragment_.GetLayoutObject()));
  PaintFillLayers(paint_info, background_color,
                  box_fragment_.Style().BackgroundLayers(), paint_rect,
                  geometry, bleed_avoidance);
}

bool NGBoxFragmentPainter::IsInSelfHitTestingPhase(HitTestAction action) const {
  // TODO(layout-dev): We should set an IsContainingBlock flag on
  // NGPhysicalBoxFragment, instead of routing back to LayoutObject.
  const LayoutObject* layout_object = box_fragment_.GetLayoutObject();
  if (layout_object->IsBox())
    return ToLayoutBox(layout_object)->IsInSelfHitTestingPhase(action);
  return action == kHitTestForeground;
}

bool NGBoxFragmentPainter::NodeAtPoint(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    const LayoutPoint& accumulated_offset_for_legacy,
    HitTestAction action) {
  // TODO(eae): Switch to using NG geometry types.
  LayoutSize offset;
  if (box_fragment_.PhysicalFragment().IsPlacedByLayoutNG()) {
    offset =
        LayoutSize(box_fragment_.Offset().left, box_fragment_.Offset().top);
  } else {
    LayoutPoint location =
        ToLayoutBox(box_fragment_.PhysicalFragment().GetLayoutObject())
            ->Location();
    offset = LayoutSize(location.X(), location.Y());
  }
  LayoutPoint adjusted_location = accumulated_offset + offset;
  LayoutSize size(box_fragment_.Size().width, box_fragment_.Size().height);
  const ComputedStyle& style = box_fragment_.Style();

  bool hit_test_self = IsInSelfHitTestingPhase(action);

  // TODO(layout-dev): Add support for hit testing overflow controls once we
  // overflow has been implemented.
  // if (hit_test_self && HasOverflowClip() &&
  //   HitTestOverflowControl(result, location_in_container, adjusted_location))
  // return true;

  bool skip_children = false;
  if (box_fragment_.ShouldClipOverflow()) {
    // PaintLayer::HitTestContentsForFragments checked the fragments'
    // foreground rect for intersection if a layer is self painting,
    // so only do the overflow clip check here for non-self-painting layers.
    if (!box_fragment_.HasSelfPaintingLayer() &&
        !location_in_container.Intersects(PhysicalFragment().OverflowClipRect(
            adjusted_location, kExcludeOverlayScrollbarSizeForHitTesting))) {
      skip_children = true;
    }
    if (!skip_children && style.HasBorderRadius()) {
      LayoutRect bounds_rect(adjusted_location, size);
      skip_children = !location_in_container.Intersects(
          style.GetRoundedInnerBorderFor(bounds_rect));
    }
  }

  // TODO(layout-dev): Accumulate |accumulated_offset_for_legacy| properly.
  LayoutPoint adjusted_location_for_legacy =
      accumulated_offset_for_legacy + offset;
  if (!skip_children &&
      HitTestChildren(result, box_fragment_.Children(), location_in_container,
                      adjusted_location, adjusted_location_for_legacy,
                      action)) {
    return true;
  }

  if (style.HasBorderRadius() &&
      HitTestClippedOutByBorder(location_in_container, adjusted_location))
    return false;

  // Now hit test ourselves.
  if (hit_test_self && VisibleToHitTestRequest(result.GetHitTestRequest())) {
    LayoutRect bounds_rect(adjusted_location, size);
    if (location_in_container.Intersects(bounds_rect)) {
      Node* node = box_fragment_.GetNode();
      if (!result.InnerNode() && node) {
        LayoutPoint point =
            location_in_container.Point() - ToLayoutSize(adjusted_location);
        result.SetNodeAndPosition(node, point);
      }
      if (result.AddNodeToListBasedTestResult(node, location_in_container,
                                              bounds_rect) == kStopHitTesting) {
        return true;
      }
    }
  }

  return false;
}

bool NGBoxFragmentPainter::VisibleToHitTestRequest(
    const HitTestRequest& request) const {
  return box_fragment_.Style().Visibility() == EVisibility::kVisible &&
         (request.IgnorePointerEventsNone() ||
          box_fragment_.Style().PointerEvents() != EPointerEvents::kNone) &&
         !(box_fragment_.GetNode() && box_fragment_.GetNode()->IsInert());
}

bool NGBoxFragmentPainter::HitTestTextFragment(
    HitTestResult& result,
    const NGPaintFragment& text_paint_fragment,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset) {
  const NGPhysicalFragment& text_fragment =
      text_paint_fragment.PhysicalFragment();
  LayoutSize offset(text_fragment.Offset().left, text_fragment.Offset().top);
  LayoutPoint adjusted_location = accumulated_offset + offset;
  LayoutSize size(text_fragment.Size().width, text_fragment.Size().height);
  LayoutRect border_rect(adjusted_location, size);
  const ComputedStyle& style = text_fragment.Style();

  if (style.HasBorderRadius()) {
    FloatRoundedRect border = style.GetRoundedBorderFor(
        border_rect,
        text_fragment.BorderEdges() & NGBorderEdges::Physical::kLeft,
        text_fragment.BorderEdges() & NGBorderEdges::Physical::kRight);
    if (!location_in_container.Intersects(border))
      return false;
  }

  // TODO(layout-dev): Clip to line-top/bottom.
  LayoutRect rect = LayoutRect(PixelSnappedIntRect(border_rect));
  if (VisibleToHitTestRequest(result.GetHitTestRequest()) &&
      location_in_container.Intersects(rect)) {
    Node* node = text_fragment.GetNode();
    if (!result.InnerNode() && node) {
      LayoutPoint point =
          location_in_container.Point() - ToLayoutSize(accumulated_offset) -
          offset +
          text_paint_fragment.InlineOffsetToContainerBox().ToLayoutPoint();
      result.SetNodeAndPosition(node, point);
    }

    if (result.AddNodeToListBasedTestResult(node, location_in_container,
                                            rect) == kStopHitTesting) {
      return true;
    }
  }

  return false;
}

bool NGBoxFragmentPainter::HitTestChildren(
    HitTestResult& result,
    const Vector<std::unique_ptr<NGPaintFragment>>& children,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    const LayoutPoint& accumulated_offset_for_legacy,
    HitTestAction action) {
  for (auto iter = children.rbegin(); iter != children.rend(); iter++) {
    const std::unique_ptr<NGPaintFragment>& child = *iter;
    if (child->HasSelfPaintingLayer())
      continue;

    const NGPhysicalFragment& fragment = child->PhysicalFragment();
    bool stop_hit_testing = false;
    if (fragment.Type() == NGPhysicalFragment::kFragmentBox) {
      if (FragmentRequiresLegacyFallback(fragment)) {
        stop_hit_testing = fragment.GetLayoutObject()->NodeAtPoint(
            result, location_in_container, accumulated_offset_for_legacy,
            action);
      } else {
        // TODO(layout-dev): Accumulate |accumulated_offset_for_legacy|
        // properly.
        stop_hit_testing = NGBoxFragmentPainter(*child).NodeAtPoint(
            result, location_in_container, accumulated_offset,
            accumulated_offset_for_legacy, action);
      }

    } else if (fragment.Type() == NGPhysicalFragment::kFragmentLineBox) {
      const LayoutSize line_box_offset(fragment.Offset().left,
                                       fragment.Offset().top);
      const LayoutPoint adjusted_offset = accumulated_offset + line_box_offset;
      // TODO(layout-dev): Accumulate |accumulated_offset_for_legacy| properly.
      stop_hit_testing = HitTestChildren(result, child->Children(),
                                         location_in_container, adjusted_offset,
                                         accumulated_offset_for_legacy, action);

    } else if (fragment.Type() == NGPhysicalFragment::kFragmentText) {
      // TODO(eae): Should this hit test on the text itself or the containing
      // node?
      stop_hit_testing = HitTestTextFragment(
          result, *child, location_in_container, accumulated_offset);
    }
    if (stop_hit_testing)
      return true;
  }

  return false;
}

bool NGBoxFragmentPainter::HitTestClippedOutByBorder(
    const HitTestLocation& location_in_container,
    const LayoutPoint& border_box_location) const {
  const ComputedStyle& style = box_fragment_.Style();
  LayoutRect rect =
      LayoutRect(LayoutPoint(), PhysicalFragment().Size().ToLayoutSize());
  rect.MoveBy(border_box_location);
  return !location_in_container.Intersects(style.GetRoundedBorderFor(rect));
}

LayoutPoint NGBoxFragmentPainter::FlipForWritingModeForChild(
    const NGPhysicalFragment& child_fragment,
    const LayoutPoint& offset) {
  if (!PhysicalFragment().Style().IsFlippedBlocksWritingMode())
    return offset;

  LayoutPoint flipped_offset;
  LayoutObject* child_layout_object = child_fragment.GetLayoutObject();
  if (!AdjustPaintOffsetScope::WillUseLegacyLocation(
          ToLayoutBox(child_layout_object)))
    return offset;
  LayoutObject* container_layout_object = box_fragment_.GetLayoutObject();
  DCHECK(child_layout_object->IsBox());
  if (container_layout_object->IsLayoutBlock()) {
    flipped_offset = ToLayoutBlock(container_layout_object)
                         ->FlipForWritingModeForChild(
                             ToLayoutBox(child_layout_object), offset);
  } else if (container_layout_object->IsInline()) {
    // Reimplementation of LayoutBox::FlipForWritingModeForChild because
    // LayoutInline cannot be an argument to FlipForWritingModeForChild
    NGPhysicalSize container_size = PhysicalFragment().Size();
    LayoutSize child_size = ToLayoutBox(child_layout_object)->Size();
    LayoutPoint child_location = ToLayoutBox(child_layout_object)->Location();
    flipped_offset =
        LayoutPoint(offset.X() + container_size.width - child_size.Width() -
                        (2 * child_location.X()),
                    offset.Y());
  }
  return flipped_offset;
}

const NGPhysicalBoxFragment& NGBoxFragmentPainter::PhysicalFragment() const {
  return static_cast<const NGPhysicalBoxFragment&>(
      box_fragment_.PhysicalFragment());
}

}  // namespace blink
