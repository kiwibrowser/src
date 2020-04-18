// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/effect_paint_property_node.h"

namespace blink {

EffectPaintPropertyNode* EffectPaintPropertyNode::Root() {
  DEFINE_STATIC_REF(EffectPaintPropertyNode, root,
                    (EffectPaintPropertyNode::Create(
                        nullptr, State{TransformPaintPropertyNode::Root(),
                                       ClipPaintPropertyNode::Root()})));
  return root;
}

FloatRect EffectPaintPropertyNode::MapRect(const FloatRect& input_rect) const {
  FloatRect rect = input_rect;
  rect.MoveBy(-state_.paint_offset);
  FloatRect result = state_.filter.MapRect(rect);
  result.MoveBy(state_.paint_offset);
  return result;
}

std::unique_ptr<JSONObject> EffectPaintPropertyNode::ToJSON() const {
  auto json = JSONObject::Create();
  if (Parent())
    json->SetString("parent", String::Format("%p", Parent()));
  json->SetString("localTransformSpace",
                  String::Format("%p", state_.local_transform_space.get()));
  json->SetString("outputClip", String::Format("%p", state_.output_clip.get()));
  if (state_.color_filter != kColorFilterNone)
    json->SetInteger("colorFilter", state_.color_filter);
  if (!state_.filter.IsEmpty())
    json->SetString("filter", state_.filter.ToString());
  if (state_.opacity != 1.0f)
    json->SetDouble("opacity", state_.opacity);
  if (state_.blend_mode != SkBlendMode::kSrcOver)
    json->SetString("blendMode", SkBlendMode_Name(state_.blend_mode));
  if (state_.direct_compositing_reasons != CompositingReason::kNone) {
    json->SetString(
        "directCompositingReasons",
        CompositingReason::ToString(state_.direct_compositing_reasons));
  }
  if (state_.compositor_element_id) {
    json->SetString("compositorElementId",
                    state_.compositor_element_id.ToString().c_str());
  }
  if (state_.paint_offset != FloatPoint())
    json->SetString("paintOffset", state_.paint_offset.ToString());
  return json;
}

}  // namespace blink
