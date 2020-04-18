// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_model_object_painter.h"

#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/line/root_inline_box.h"
#include "third_party/blink/renderer/core/paint/background_image_geometry.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/geometry/layout_point.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect_outsets.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"

namespace blink {

namespace {

Node* GeneratingNodeForObject(const LayoutBoxModelObject& box_model) {
  Node* node = nullptr;
  const LayoutObject* layout_object = &box_model;
  for (; layout_object && !node; layout_object = layout_object->Parent()) {
    node = layout_object->GeneratingNode();
  }
  return node;
}

LayoutSize LogicalOffsetOnLine(const InlineFlowBox& flow_box) {
  // Compute the offset of the passed flow box when seen as part of an
  // unbroken continuous strip (c.f box-decoration-break: slice.)
  LayoutUnit logical_offset_on_line;
  if (flow_box.IsLeftToRightDirection()) {
    for (const InlineFlowBox* curr = flow_box.PrevForSameLayoutObject(); curr;
         curr = curr->PrevForSameLayoutObject())
      logical_offset_on_line += curr->LogicalWidth();
  } else {
    for (const InlineFlowBox* curr = flow_box.NextForSameLayoutObject(); curr;
         curr = curr->NextForSameLayoutObject())
      logical_offset_on_line += curr->LogicalWidth();
  }
  LayoutSize logical_offset(logical_offset_on_line, LayoutUnit());
  return flow_box.IsHorizontal() ? logical_offset
                                 : logical_offset.TransposedSize();
}

}  // anonymous namespace

BoxModelObjectPainter::BoxModelObjectPainter(const LayoutBoxModelObject& box,
                                             const InlineFlowBox* flow_box)
    : BoxPainterBase(box,
                     &box.GetDocument(),
                     box.StyleRef(),
                     GeneratingNodeForObject(box),
                     box.BorderBoxOutsets(),
                     box.PaddingOutsets(),
                     box.Layer()),
      box_model_(box),
      flow_box_(flow_box) {}

bool BoxModelObjectPainter::
    IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
        const LayoutBoxModelObject* box_model_,
        const PaintInfo& paint_info) {
  return paint_info.PaintFlags() & kPaintLayerPaintingOverflowContents &&
         !(paint_info.PaintFlags() &
           kPaintLayerPaintingCompositingBackgroundPhase) &&
         box_model_ == paint_info.PaintContainer();
}

void BoxModelObjectPainter::PaintTextClipMask(GraphicsContext& context,
                                              const IntRect& mask_rect,
                                              const LayoutPoint& paint_offset) {
  PaintInfo paint_info(context, mask_rect, PaintPhase::kTextClip,
                       kGlobalPaintNormalPhase, 0);
  if (flow_box_) {
    LayoutSize local_offset = ToLayoutSize(flow_box_->Location());
    if (box_model_.StyleRef().BoxDecorationBreak() ==
        EBoxDecorationBreak::kSlice) {
      local_offset -= LogicalOffsetOnLine(*flow_box_);
    }
    const RootInlineBox& root = flow_box_->Root();
    flow_box_->Paint(paint_info, paint_offset - local_offset, root.LineTop(),
                     root.LineBottom());
  } else {
    // FIXME: this should only have an effect for the line box list within
    // |box_model_|. Change this to create a LineBoxListPainter directly.
    LayoutSize local_offset = box_model_.IsBox()
                                  ? ToLayoutBox(&box_model_)->LocationOffset()
                                  : LayoutSize();
    box_model_.Paint(paint_info, paint_offset - local_offset);
  }
}

FloatRoundedRect BoxModelObjectPainter::GetBackgroundRoundedRect(
    const LayoutRect& border_rect,
    const LayoutSize& flow_box_size,
    bool include_logical_left_edge,
    bool include_logical_right_edge) const {
  FloatRoundedRect border = BoxPainterBase::GetBackgroundRoundedRect(
      border_rect, flow_box_size, include_logical_left_edge,
      include_logical_right_edge);
  if (flow_box_ && (flow_box_->NextForSameLayoutObject() ||
                    flow_box_->PrevForSameLayoutObject())) {
    FloatRoundedRect segment_border = box_model_.StyleRef().GetRoundedBorderFor(
        LayoutRect(LayoutPoint(), LayoutSize(FlooredIntSize(flow_box_size))),
        include_logical_left_edge, include_logical_right_edge);
    border.SetRadii(segment_border.GetRadii());
  }
  return border;
}

LayoutRect BoxModelObjectPainter::AdjustForScrolledContent(
    const PaintInfo& paint_info,
    const BoxPainterBase::FillLayerInfo& info,
    const LayoutRect& rect) {
  LayoutRect scrolled_paint_rect = rect;
  GraphicsContext& context = paint_info.context;
  if (info.is_clipped_with_local_scrolling &&
      !IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          &box_model_, paint_info)) {
    // Clip to the overflow area.
    const LayoutBox& this_box = ToLayoutBox(box_model_);
    // TODO(chrishtr): this should be pixel-snapped.
    context.Clip(FloatRect(this_box.OverflowClipRect(rect.Location())));

    // Adjust the paint rect to reflect a scrolled content box with borders at
    // the ends.
    IntSize offset = this_box.ScrolledContentOffset();
    scrolled_paint_rect.Move(-offset);
    LayoutRectOutsets border = BorderOutsets(info);
    scrolled_paint_rect.SetWidth(border.Left() + this_box.ScrollWidth() +
                                 border.Right());
    scrolled_paint_rect.SetHeight(this_box.BorderTop() +
                                  this_box.ScrollHeight() +
                                  this_box.BorderBottom());
  }
  return scrolled_paint_rect;
}

BoxPainterBase::FillLayerInfo BoxModelObjectPainter::GetFillLayerInfo(
    const Color& color,
    const FillLayer& bg_layer,
    BackgroundBleedAvoidance bleed_avoidance) const {
  return BoxPainterBase::FillLayerInfo(
      box_model_.GetDocument(), box_model_.StyleRef(),
      box_model_.HasOverflowClip(), color, bg_layer, bleed_avoidance,
      (flow_box_ ? flow_box_->IncludeLogicalLeftEdge() : true),
      (flow_box_ ? flow_box_->IncludeLogicalRightEdge() : true));
}

}  // namespace blink
