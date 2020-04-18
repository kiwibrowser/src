// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/scroll_hit_test_display_item.h"

#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

ScrollHitTestDisplayItem::ScrollHitTestDisplayItem(
    const DisplayItemClient& client,
    Type type,
    scoped_refptr<const TransformPaintPropertyNode> scroll_offset_node)
    : DisplayItem(client, type, sizeof(*this)),
      scroll_offset_node_(std::move(scroll_offset_node)) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  DCHECK(IsScrollHitTestType(type));
  // The scroll offset transform node should have an associated scroll node.
  DCHECK(scroll_offset_node_->ScrollNode());
}

ScrollHitTestDisplayItem::~ScrollHitTestDisplayItem() = default;

void ScrollHitTestDisplayItem::Replay(GraphicsContext&) const {
  NOTREACHED();
}

void ScrollHitTestDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList&) const {
  NOTREACHED();
}

bool ScrollHitTestDisplayItem::Equals(const DisplayItem& other) const {
  return DisplayItem::Equals(other) &&
         &scroll_node() ==
             &static_cast<const ScrollHitTestDisplayItem&>(other).scroll_node();
}

#if DCHECK_IS_ON()
void ScrollHitTestDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  DisplayItem::PropertiesAsJSON(json);
  json.SetString("scrollOffsetNode",
                 String::Format("%p", scroll_offset_node_.get()));
}
#endif

void ScrollHitTestDisplayItem::Record(
    GraphicsContext& context,
    const DisplayItemClient& client,
    DisplayItem::Type type,
    scoped_refptr<const TransformPaintPropertyNode> scroll_offset_node) {
  PaintController& paint_controller = context.GetPaintController();

  // The scroll hit test should be in the non-scrolled transform space and
  // therefore should not be scrolled by the associated scroll offset.
  DCHECK_NE(paint_controller.CurrentPaintChunkProperties().Transform(),
            scroll_offset_node.get());

  if (paint_controller.DisplayItemConstructionIsDisabled())
    return;

  paint_controller.CreateAndAppend<ScrollHitTestDisplayItem>(
      client, type, std::move(scroll_offset_node));
}

}  // namespace blink
