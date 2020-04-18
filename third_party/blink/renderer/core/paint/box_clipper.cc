// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_clipper.h"

#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_root.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

// Clips for boxes are applied by the box's PaintLayerClipper, if the box has
// a self-painting PaintLayer. Otherwise the box clips itself.
// Note that this method is very similar to
// PaintLayerClipper::shouldClipOverflow for that reason.
//
// An exception is control clip, which is currently never applied by
// PaintLayerClipper.
static bool BoxNeedsClip(const LayoutBox& box) {
  if (box.HasLayer() && box.Layer()->IsSelfPaintingLayer())
    return false;
  return box.ShouldClipOverflow();
}

DISABLE_CFI_PERF
BoxClipper::BoxClipper(const LayoutBox& box,
                       const PaintInfo& paint_info,
                       const LayoutPoint& accumulated_offset,
                       ContentsClipBehavior contents_clip_behavior)
    : box_(box),
      paint_info_(paint_info),
      clip_type_(DisplayItem::kUninitializedType) {
  DCHECK(paint_info_.phase != PaintPhase::kSelfBlockBackgroundOnly &&
         paint_info_.phase != PaintPhase::kSelfOutlineOnly);

  if (paint_info_.phase == PaintPhase::kMask)
    return;

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    InitializeScopedClipProperty(paint_info.FragmentToPaint(box_), box,
                                 paint_info);
    return;
  }

  if (!BoxNeedsClip(box_))
    return;

  LayoutRect clip_rect = box_.OverflowClipRect(accumulated_offset);
  FloatRoundedRect clip_rounded_rect(0, 0, 0, 0);
  bool has_border_radius = box_.Style()->HasBorderRadius();
  if (has_border_radius) {
    clip_rounded_rect = box_.Style()->GetRoundedInnerBorderFor(
        LayoutRect(accumulated_offset, box_.Size()));
  }

  // Selection may extend beyond visual overflow, so this optimization is
  // invalid if selection is present.
  if (contents_clip_behavior == kSkipContentsClipIfPossible &&
      box.GetSelectionState() == SelectionState::kNone) {
    LayoutRect contents_visual_overflow = box_.ContentsVisualOverflowRect();
    if (contents_visual_overflow.IsEmpty())
      return;

    LayoutRect conservative_clip_rect = clip_rect;
    if (has_border_radius)
      conservative_clip_rect.Intersect(
          LayoutRect(clip_rounded_rect.RadiusCenterRect()));
    conservative_clip_rect.MoveBy(-accumulated_offset);
    if (box_.HasLayer())
      conservative_clip_rect.Move(box_.ScrolledContentOffset());
    if (conservative_clip_rect.Contains(contents_visual_overflow))
      return;
  }

  if (!paint_info_.context.GetPaintController()
           .DisplayItemConstructionIsDisabled()) {
    clip_type_ = paint_info_.DisplayItemTypeForClipping();
    Vector<FloatRoundedRect> rounded_rects;
    if (has_border_radius)
      rounded_rects.push_back(clip_rounded_rect);
    paint_info_.context.GetPaintController().CreateAndAppend<ClipDisplayItem>(
        box_, clip_type_, PixelSnappedIntRect(clip_rect), rounded_rects);
  }
}

BoxClipper::~BoxClipper() {
  if (clip_type_ == DisplayItem::kUninitializedType)
    return;

  DCHECK(BoxNeedsClip(box_));
  paint_info_.context.GetPaintController().EndItem<EndClipDisplayItem>(
      box_, DisplayItem::ClipTypeToEndClipType(clip_type_));
}

}  // namespace blink
