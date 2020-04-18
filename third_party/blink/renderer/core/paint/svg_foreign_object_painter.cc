// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/svg_foreign_object_painter.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_foreign_object.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/paint/block_painter.h"
#include "third_party/blink/renderer/core/paint/float_clip_recorder.h"
#include "third_party/blink/renderer/core/paint/object_painter.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"
#include "third_party/blink/renderer/core/paint/svg_paint_context.h"

namespace blink {

namespace {

class BlockPainterDelegate : public LayoutBlock {
 public:
  BlockPainterDelegate(const LayoutSVGForeignObject& layout_svg_foreign_object)
      : LayoutBlock(nullptr),
        layout_svg_foreign_object_(layout_svg_foreign_object) {}

 private:
  void Paint(const PaintInfo& paint_info,
             const LayoutPoint& paint_offset) const final {
    BlockPainter(layout_svg_foreign_object_).Paint(paint_info, paint_offset);
  }
  const LayoutSVGForeignObject& layout_svg_foreign_object_;
};

}  // namespace

void SVGForeignObjectPainter::PaintLayer(const PaintInfo& paint_info) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  if (paint_info.phase != PaintPhase::kForeground &&
      paint_info.phase != PaintPhase::kSelection)
    return;

  // Early out in the case of trying to paint an image filter before
  // pre-paint has finished.
  if (!layout_svg_foreign_object_.FirstFragment().HasLocalBorderBoxProperties())
    return;

  // <foreignObject> is a replaced normal-flow stacking element.
  // See IsReplacedNormalFlowStacking in paint_layer_painter.cc.
  PaintLayerPaintingInfo layer_painting_info(
      layout_svg_foreign_object_.Layer(),
      // Reset to an infinite cull rect, for simplicity. Otherwise
      // an adjustment would be needed for ancestor scrolling, and any
      // SVG transforms would have to be taken into account. Further,
      // cull rects under transform are intentionally reset to infinity,
      // to improve cache invalidation performance in the pre-paint tree
      // walk (see https://http://crrev.com/482854).
      LayoutRect(LayoutRect::InfiniteIntRect()),
      paint_info.GetGlobalPaintFlags(), LayoutSize());
  PaintLayerPainter(*layout_svg_foreign_object_.Layer())
      .Paint(paint_info.context, layer_painting_info, paint_info.PaintFlags());
}

void SVGForeignObjectPainter::Paint(const PaintInfo& paint_info) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    if (paint_info.phase != PaintPhase::kForeground &&
        paint_info.phase != PaintPhase::kSelection)
      return;
  }

  PaintInfo paint_info_before_filtering(paint_info);
  base::Optional<SVGTransformContext> transform_context;

  if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    paint_info_before_filtering.UpdateCullRect(
        layout_svg_foreign_object_.LocalSVGTransform());
    transform_context.emplace(paint_info_before_filtering,
                              layout_svg_foreign_object_,
                              layout_svg_foreign_object_.LocalSVGTransform());

    // In theory we should just let BlockPainter::paint() handle the clip, but
    // for now we don't allow normal overflow clip for LayoutSVGBlock, so we
    // have to apply clip manually. See LayoutSVGBlock::allowsOverflowClip() for
    // details.
    base::Optional<FloatClipRecorder> clip_recorder;
    base::Optional<ScopedPaintChunkProperties> scoped_paint_chunk_properties;
    if (SVGLayoutSupport::IsOverflowHidden(layout_svg_foreign_object_)) {
      clip_recorder.emplace(paint_info_before_filtering.context,
                            layout_svg_foreign_object_,
                            paint_info_before_filtering.phase,
                            FloatRect(layout_svg_foreign_object_.FrameRect()));
    }
  }

  SVGPaintContext paint_context(layout_svg_foreign_object_,
                                paint_info_before_filtering);
  if (paint_context.GetPaintInfo().phase == PaintPhase::kForeground &&
      !paint_context.ApplyClipMaskAndFilterIfNecessary())
    return;

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    BlockPainter(layout_svg_foreign_object_).Paint(paint_info, LayoutPoint());
  } else {
    // Paint all phases of FO elements atomically as though the FO element
    // established its own stacking context.  The delegate forwards calls to
    // paint() in LayoutObject::paintAllPhasesAtomically() to
    // BlockPainter::paint(), instead of m_layoutSVGForeignObject.paint() (which
    // would call this method again).
    BlockPainterDelegate delegate(layout_svg_foreign_object_);
    ObjectPainter(delegate).PaintAllPhasesAtomically(
        paint_context.GetPaintInfo(), LayoutPoint());
  }
}

}  // namespace blink
