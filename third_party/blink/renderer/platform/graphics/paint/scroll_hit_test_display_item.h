// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_HIT_TEST_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_HIT_TEST_DISPLAY_ITEM_H_

#include "base/memory/ref_counted.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class GraphicsContext;

// Placeholder display item for creating a special cc::Layer marked as being
// scrollable in PaintArtifactCompositor. A display item is needed because
// scroll hit testing must be in paint order.
//
// The scroll hit test display item keeps track of the scroll offset translation
// node which also has a reference to the associated scroll node. The scroll hit
// test display item should be in the non-scrolled transform space and therefore
// should not be scrolled by the associated scroll offset transform.
class PLATFORM_EXPORT ScrollHitTestDisplayItem final : public DisplayItem {
 public:
  ScrollHitTestDisplayItem(
      const DisplayItemClient&,
      Type,
      scoped_refptr<const TransformPaintPropertyNode> scroll_offset_node);
  ~ScrollHitTestDisplayItem() override;

  const TransformPaintPropertyNode& scroll_offset_node() const {
    return *scroll_offset_node_;
  }

  // DisplayItem
  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;
  bool Equals(const DisplayItem&) const override;
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const override;
#endif

  // Create and append a ScrollHitTestDisplayItem onto the context. This is
  // similar to a recorder class (e.g., DrawingRecorder) but just emits a single
  // item.
  static void Record(
      GraphicsContext&,
      const DisplayItemClient&,
      DisplayItem::Type,
      scoped_refptr<const TransformPaintPropertyNode> scroll_offset_node);

 private:
  const ScrollPaintPropertyNode& scroll_node() const {
    return *scroll_offset_node_->ScrollNode();
  }

  scoped_refptr<const TransformPaintPropertyNode> scroll_offset_node_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_SCROLL_HIT_TEST_DISPLAY_ITEM_H_
