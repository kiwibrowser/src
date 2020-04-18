// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/clip_paint_property_node.h"

#include "third_party/blink/renderer/platform/geometry/layout_rect.h"

namespace blink {

ClipPaintPropertyNode* ClipPaintPropertyNode::Root() {
  DEFINE_STATIC_REF(
      ClipPaintPropertyNode, root,
      (ClipPaintPropertyNode::Create(
          nullptr, State{TransformPaintPropertyNode::Root(),
                         FloatRoundedRect(LayoutRect::InfiniteIntRect())})));
  return root;
}

std::unique_ptr<JSONObject> ClipPaintPropertyNode::ToJSON() const {
  auto json = JSONObject::Create();
  if (Parent())
    json->SetString("parent", String::Format("%p", Parent()));
  json->SetString("localTransformSpace",
                  String::Format("%p", state_.local_transform_space.get()));
  json->SetString("rect", state_.clip_rect.ToString());
  if (state_.clip_rect_excluding_overlay_scrollbars) {
    json->SetString("rectExcludingOverlayScrollbars",
                    state_.clip_rect_excluding_overlay_scrollbars->ToString());
  }
  if (state_.clip_path) {
    json->SetBoolean("hasClipPath", true);
  }
  if (state_.direct_compositing_reasons != CompositingReason::kNone) {
    json->SetString(
        "directCompositingReasons",
        CompositingReason::ToString(state_.direct_compositing_reasons));
  }
  return json;
}

size_t ClipPaintPropertyNode::CacheMemoryUsageInBytes() const {
  size_t total_bytes = sizeof(*this);
  if (geometry_mapper_clip_cache_)
    total_bytes += sizeof(*geometry_mapper_clip_cache_);
  if (Parent())
    total_bytes += Parent()->CacheMemoryUsageInBytes();
  return total_bytes;
}

}  // namespace blink
