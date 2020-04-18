// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/line_box_list_painter.h"

#include "third_party/blink/renderer/core/layout/api/line_layout_box_model.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/line/inline_flow_box.h"
#include "third_party/blink/renderer/core/layout/line/line_box_list.h"
#include "third_party/blink/renderer/core/layout/line/root_inline_box.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

static void AddPDFURLRectsForInlineChildrenRecursively(
    const LayoutObject& layout_object,
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  for (LayoutObject* child = layout_object.SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (!child->IsLayoutInline() ||
        ToLayoutBoxModelObject(child)->HasSelfPaintingLayer())
      continue;
    ObjectPainter(*child).AddPDFURLRectIfNeeded(paint_info, paint_offset);
    AddPDFURLRectsForInlineChildrenRecursively(*child, paint_info,
                                               paint_offset);
  }
}

void LineBoxListPainter::Paint(const LayoutBoxModelObject& layout_object,
                               const PaintInfo& paint_info,
                               const LayoutPoint& paint_offset) const {
  DCHECK(!ShouldPaintSelfOutline(paint_info.phase) &&
         !ShouldPaintDescendantOutlines(paint_info.phase));

  // Only paint during the foreground/selection phases.
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection &&
      paint_info.phase != PaintPhase::kTextClip &&
      paint_info.phase != PaintPhase::kMask)
    return;

  // The only way an inline could paint like this is if it has a layer.
  DCHECK(layout_object.IsLayoutBlock() ||
         (layout_object.IsLayoutInline() && layout_object.HasLayer()));

  if (paint_info.phase == PaintPhase::kForeground && paint_info.IsPrinting())
    AddPDFURLRectsForInlineChildrenRecursively(layout_object, paint_info,
                                               paint_offset);

  // If we have no lines then we have no work to do.
  if (!line_box_list_.First())
    return;

  if (!line_box_list_.AnyLineIntersectsRect(
          LineLayoutBoxModel(const_cast<LayoutBoxModelObject*>(&layout_object)),
          paint_info.GetCullRect(), paint_offset))
    return;

  // See if our root lines intersect with the dirty rect. If so, then we paint
  // them. Note that boxes can easily overlap, so we can't make any assumptions
  // based off positions of our first line box or our last line box.
  for (InlineFlowBox* curr : line_box_list_) {
    if (line_box_list_.LineIntersectsDirtyRect(
            LineLayoutBoxModel(
                const_cast<LayoutBoxModelObject*>(&layout_object)),
            curr, paint_info.GetCullRect(), paint_offset)) {
      RootInlineBox& root = curr->Root();
      curr->Paint(paint_info, paint_offset, root.LineTop(), root.LineBottom());
    }
  }
}

}  // namespace blink
