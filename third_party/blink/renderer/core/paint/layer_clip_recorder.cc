// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/layer_clip_recorder.h"

#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/clip_rect.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

LayerClipRecorder::LayerClipRecorder(GraphicsContext& graphics_context,
                                     const PaintLayer& paint_layer,
                                     DisplayItem::Type clip_type,
                                     const ClipRect& clip_rect,
                                     const PaintLayer* clip_root,
                                     const LayoutPoint& fragment_offset,
                                     PaintLayerFlags paint_flags,
                                     const DisplayItemClient& client,
                                     BorderRadiusClippingRule rule)
    : graphics_context_(graphics_context),
      client_(client),
      clip_type_(clip_type) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  IntRect snapped_clip_rect = PixelSnappedIntRect(clip_rect.Rect());
#if DCHECK_IS_ON
  // In SPv175+ mode, clip rects are pre-snapped.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    DCHECK(FloatRect(snapped_clip_rect) == clip_rect.Rect());
#endif
  bool painting_masks =
      (paint_flags & kPaintLayerPaintingChildClippingMaskPhase ||
       paint_flags & kPaintLayerPaintingAncestorClippingMaskPhase);
  Vector<FloatRoundedRect> rounded_rects;
  if (clip_root && (clip_rect.HasRadius() || painting_masks)) {
    CollectRoundedRectClips(paint_layer, clip_root, fragment_offset,
                            painting_masks, rule, rounded_rects);
  }

  graphics_context_.GetPaintController().CreateAndAppend<ClipDisplayItem>(
      client_, clip_type_, snapped_clip_rect, rounded_rects);
}

static bool InContainingBlockChain(const PaintLayer* start_layer,
                                   const PaintLayer* end_layer) {
  if (start_layer == end_layer)
    return true;

  LayoutView* view = start_layer->GetLayoutObject().View();
  for (const LayoutBlock* current_block =
           start_layer->GetLayoutObject().ContainingBlock();
       current_block && current_block != view;
       current_block = current_block->ContainingBlock()) {
    if (current_block->Layer() == end_layer)
      return true;
  }

  return false;
}

void LayerClipRecorder::CollectRoundedRectClips(
    const PaintLayer& paint_layer,
    const PaintLayer* clip_root,
    const LayoutPoint& fragment_offset,
    bool cross_composited_scrollers,
    BorderRadiusClippingRule rule,
    Vector<FloatRoundedRect>& rounded_rect_clips) {
  // If the clip rect has been tainted by a border radius, then we have to walk
  // up our layer chain applying the clips from any layers with overflow. The
  // condition for being able to apply these clips is that the overflow object
  // be in our containing block chain so we check that also.
  for (const PaintLayer* layer = rule == kIncludeSelfForBorderRadius
                                     ? &paint_layer
                                     : paint_layer.Parent();
       layer; layer = layer->Parent()) {
    // Composited scrolling layers handle border-radius clip in the compositor
    // via a mask layer. We do not want to apply a border-radius clip to the
    // layer contents itself, because that would require re-rastering every
    // frame to update the clip. We only want to make sure that the mask layer
    // is properly clipped so that it can in turn clip the scrolled contents in
    // the compositor.
    if (!cross_composited_scrollers && layer->NeedsCompositedScrolling())
      break;

    // Collect clips for embedded content despite their lack of overflow,
    // because in practice they do need to clip. However, the clip is only
    // used when painting child clipping masks to avoid clipping out border
    // decorations.
    if ((layer->GetLayoutObject().HasOverflowClip() ||
         layer->GetLayoutObject().IsLayoutEmbeddedContent()) &&
        layer->GetLayoutObject().Style()->HasBorderRadius() &&
        InContainingBlockChain(&paint_layer, layer)) {
      LayoutPoint delta;
      if (layer->EnclosingPaginationLayer() ==
          paint_layer.EnclosingPaginationLayer())
        delta.MoveBy(fragment_offset);
      layer->ConvertToLayerCoords(clip_root, delta);

      // The PaintLayer's size is pixel-snapped if it is a LayoutBox. We can't
      // use a pre-snapped border rect for clipping, since
      // getRoundedInnerBorderFor assumes it has not been snapped yet.
      LayoutSize size(layer->GetLayoutBox()
                          ? ToLayoutBox(layer->GetLayoutObject()).Size()
                          : LayoutSize(layer->Size()));
      rounded_rect_clips.push_back(
          layer->GetLayoutObject().Style()->GetRoundedInnerBorderFor(
              LayoutRect(delta, size)));
    }

    if (layer == clip_root)
      break;
  }
}

LayerClipRecorder::~LayerClipRecorder() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return;
  graphics_context_.GetPaintController().EndItem<EndClipDisplayItem>(
      client_, DisplayItem::ClipTypeToEndClipType(clip_type_));
}

}  // namespace blink
