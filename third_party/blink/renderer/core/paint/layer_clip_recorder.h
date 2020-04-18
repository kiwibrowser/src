// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LAYER_CLIP_RECORDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LAYER_CLIP_RECORDER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painting_info.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ClipRect;
class GraphicsContext;

class CORE_EXPORT LayerClipRecorder {
  USING_FAST_MALLOC(LayerClipRecorder);

 public:
  enum BorderRadiusClippingRule {
    kIncludeSelfForBorderRadius,
    kDoNotIncludeSelfForBorderRadius
  };

  // Set rounded clip rectangles defined by border radii all the way from the
  // PaintLayerPaintingInfo "root" layer down to the specified layer (or the
  // parent of said layer, in case BorderRadiusClippingRule says to skip self).
  // fragmentOffset is used for multicol, to specify the translation required to
  // get from flow thread coordinates to visual coordinates for a certain
  // column.
  // FIXME: The BorderRadiusClippingRule parameter is really useless now. If we
  // want to skip self,
  // why not just supply the parent layer as the first parameter instead?
  // FIXME: The ClipRect passed is in visual coordinates (not flow thread
  // coordinates), but at the same time we pass a fragmentOffset, so that we can
  // translate from flow thread coordinates to visual coordinates. This may look
  // rather confusing/redundant, but it is needed for rounded border clipping.
  // Would be nice to clean up this.
  explicit LayerClipRecorder(
      GraphicsContext&,
      const PaintLayer&,
      DisplayItem::Type,
      const ClipRect&,
      const PaintLayer* clip_root,
      const LayoutPoint& fragment_offset,
      PaintLayerFlags,
      const DisplayItemClient&,
      BorderRadiusClippingRule = kIncludeSelfForBorderRadius);

  ~LayerClipRecorder();

  // Build a vector of the border radius clips that should be applied to
  // the given PaintLayer, walking up the paint layer tree to the clip_root.
  // The fragment_offset is an offset to apply to the clip to position it
  // in the required clipping coordinates (for cases when the painting
  // coordinate system is offset from the layer coordinate system).
  // cross_composited_scrollers should be true when the search for clips should
  // continue even if the clipping layer is painting into a composited scrolling
  // layer, as when painting a mask for a child of the scroller.
  // The BorderRadiusClippingRule defines whether clips on the PaintLayer itself
  // are included. Output is appended to rounded_rect_clips.
  static void CollectRoundedRectClips(
      const PaintLayer&,
      const PaintLayer* clip_root,
      const LayoutPoint& fragment_offset,
      bool cross_composited_scrollers,
      BorderRadiusClippingRule,
      Vector<FloatRoundedRect>& rounded_rect_clips);

 private:
  GraphicsContext& graphics_context_;
  const DisplayItemClient& client_;
  DisplayItem::Type clip_type_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LAYER_CLIP_RECORDER_H_
