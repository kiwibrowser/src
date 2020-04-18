// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class InlineBox;
class LayoutBlock;
class LayoutBox;
class LayoutFlexibleBox;
class LayoutPoint;

class BlockPainter {
  STACK_ALLOCATED();

 public:
  BlockPainter(const LayoutBlock& block) : layout_block_(block) {}

  void Paint(const PaintInfo&, const LayoutPoint& paint_offset);
  void PaintObject(const PaintInfo&, const LayoutPoint&);
  void PaintContents(const PaintInfo&, const LayoutPoint&);
  void PaintChildren(const PaintInfo&, const LayoutPoint&);
  void PaintChild(const LayoutBox&, const PaintInfo&, const LayoutPoint&);
  void PaintOverflowControlsIfNeeded(const PaintInfo&, const LayoutPoint&);

  // See ObjectPainter::paintAllPhasesAtomically().
  void PaintAllChildPhasesAtomically(const LayoutBox&,
                                     const PaintInfo&,
                                     const LayoutPoint&);
  static void PaintChildrenOfFlexibleBox(const LayoutFlexibleBox&,
                                         const PaintInfo&,
                                         const LayoutPoint& paint_offset);
  static void PaintInlineBox(const InlineBox&,
                             const PaintInfo&,
                             const LayoutPoint& paint_offset);

  // The adjustedPaintOffset should include the location (offset) of the object
  // itself.
  bool IntersectsPaintRect(const PaintInfo&,
                           const LayoutPoint& adjusted_paint_offset) const;

 private:
  // Paint scroll hit test placeholders in the correct paint order (see:
  // ScrollHitTestDisplayItem.h).
  void PaintScrollHitTestDisplayItem(const PaintInfo&);
  void PaintCarets(const PaintInfo&, const LayoutPoint&);

  const LayoutBlock& layout_block_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_PAINTER_H_
