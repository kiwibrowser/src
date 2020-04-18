// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_INLINE_FLOW_BOX_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_INLINE_FLOW_BOX_PAINTER_H_

#include "third_party/blink/renderer/core/style/shadow_data.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class Color;
class FillLayer;
class InlineFlowBox;
class IntRect;
class LayoutPoint;
class LayoutRect;
class LayoutSize;
class LayoutUnit;
struct PaintInfo;
class ComputedStyle;

class InlineFlowBoxPainter {
  STACK_ALLOCATED();

 public:
  InlineFlowBoxPainter(const InlineFlowBox& inline_flow_box)
      : inline_flow_box_(inline_flow_box) {}
  void Paint(const PaintInfo&,
             const LayoutPoint& paint_offset,
             const LayoutUnit line_top,
             const LayoutUnit line_bottom);

  LayoutRect FrameRectClampedToLineTopAndBottomIfNeeded() const;

 private:
  void PaintBoxDecorationBackground(const PaintInfo&,
                                    const LayoutPoint& paint_offset);
  void PaintMask(const PaintInfo&, const LayoutPoint& paint_offset);
  void PaintFillLayers(const PaintInfo&,
                       const Color&,
                       const FillLayer&,
                       const LayoutRect&,
                       SkBlendMode op = SkBlendMode::kSrcOver);
  void PaintFillLayer(const PaintInfo&,
                      const Color&,
                      const FillLayer&,
                      const LayoutRect&,
                      SkBlendMode op);
  inline bool ShouldForceIncludeLogicalEdges() const;
  inline bool IncludeLogicalLeftEdgeForBoxShadow() const;
  inline bool IncludeLogicalRightEdgeForBoxShadow() const;
  void PaintNormalBoxShadow(const PaintInfo&,
                            const ComputedStyle&,
                            const LayoutRect& paint_rect);
  void PaintInsetBoxShadow(const PaintInfo&,
                           const ComputedStyle&,
                           const LayoutRect& paint_rect);
  LayoutRect PaintRectForImageStrip(const LayoutPoint& paint_offset,
                                    const LayoutSize& frame_size,
                                    TextDirection) const;

  enum BorderPaintingType {
    kDontPaintBorders,
    kPaintBordersWithoutClip,
    kPaintBordersWithClip
  };
  BorderPaintingType GetBorderPaintType(const LayoutRect& adjusted_frame_rect,
                                        IntRect& adjusted_clip_rect) const;

  const InlineFlowBox& inline_flow_box_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_INLINE_FLOW_BOX_PAINTER_H_
