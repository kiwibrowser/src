// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_EFFECT_PAINT_PROPERTY_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_EFFECT_PAINT_PROPERTY_NODE_H_

#include "cc/layers/layer.h"
#include "third_party/blink/renderer/platform/graphics/compositor_element_id.h"
#include "third_party/blink/renderer/platform/graphics/compositor_filter_operations.h"
#include "third_party/blink/renderer/platform/graphics/graphics_types.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

// Effect nodes are abstraction of isolated groups, along with optional effects
// that can be applied to the composited output of the group.
//
// The effect tree is rooted at a node with no parent. This root node should
// not be modified.
class PLATFORM_EXPORT EffectPaintPropertyNode
    : public PaintPropertyNode<EffectPaintPropertyNode> {
 public:
  // To make it less verbose and more readable to construct and update a node,
  // a struct with default values is used to represent the state.
  struct State {
    // The local transform space serves two purposes:
    // 1. Assign a depth mapping for 3D depth sorting against other paint chunks
    //    and effects under the same parent.
    // 2. Some effects are spatial (namely blur filter and reflection), the
    //    effect parameters will be specified in the local space.
    scoped_refptr<const TransformPaintPropertyNode> local_transform_space;
    // The output of the effect can be optionally clipped when composited onto
    // the current backdrop.
    scoped_refptr<const ClipPaintPropertyNode> output_clip;
    // Optionally a number of effects can be applied to the composited output.
    // The chain of effects will be applied in the following order:
    // === Begin of effects ===
    ColorFilter color_filter = kColorFilterNone;
    CompositorFilterOperations filter;
    float opacity = 1;
    SkBlendMode blend_mode = SkBlendMode::kSrcOver;
    // === End of effects ===
    CompositingReasons direct_compositing_reasons = CompositingReason::kNone;
    CompositorElementId compositor_element_id;
    // The offset of the effect's local space in m_localTransformSpace. Some
    // effects e.g. reflection need this to apply geometry effects in the local
    // space.
    FloatPoint paint_offset = FloatPoint();

    bool operator==(const State& o) const {
      return local_transform_space == o.local_transform_space &&
             output_clip == o.output_clip && color_filter == o.color_filter &&
             filter == o.filter && opacity == o.opacity &&
             blend_mode == o.blend_mode &&
             direct_compositing_reasons == o.direct_compositing_reasons &&
             compositor_element_id == o.compositor_element_id &&
             paint_offset == o.paint_offset;
    }
  };

  // This node is really a sentinel, and does not represent a real effect.
  static EffectPaintPropertyNode* Root();

  static scoped_refptr<EffectPaintPropertyNode> Create(
      scoped_refptr<const EffectPaintPropertyNode> parent,
      State&& state) {
    return base::AdoptRef(
        new EffectPaintPropertyNode(std::move(parent), std::move(state)));
  }

  bool Update(scoped_refptr<const EffectPaintPropertyNode> parent,
              State&& state) {
    bool parent_changed = SetParent(parent);
    if (state == state_)
      return parent_changed;

    SetChanged();
    state_ = std::move(state);
    return true;
  }

  const TransformPaintPropertyNode* LocalTransformSpace() const {
    return state_.local_transform_space.get();
  }
  const ClipPaintPropertyNode* OutputClip() const {
    return state_.output_clip.get();
  }

  SkBlendMode BlendMode() const { return state_.blend_mode; }
  float Opacity() const { return state_.opacity; }
  const CompositorFilterOperations& Filter() const { return state_.filter; }
  ColorFilter GetColorFilter() const { return state_.color_filter; }

  bool HasFilterThatMovesPixels() const {
    return state_.filter.HasFilterThatMovesPixels();
  }

  FloatPoint PaintOffset() const { return state_.paint_offset; }

  // Returns a rect covering the pixels that can be affected by pixels in
  // |inputRect|. The rects are in the space of localTransformSpace.
  FloatRect MapRect(const FloatRect& input_rect) const;

  bool HasDirectCompositingReasons() const {
    return state_.direct_compositing_reasons != CompositingReason::kNone;
  }

  bool RequiresCompositingForAnimation() const {
    return state_.direct_compositing_reasons &
           CompositingReason::kComboActiveAnimation;
  }

  const CompositorElementId& GetCompositorElementId() const {
    return state_.compositor_element_id;
  }

#if DCHECK_IS_ON()
  // The clone function is used by FindPropertiesNeedingUpdate.h for recording
  // an effect node before it has been updated, to later detect changes.
  scoped_refptr<EffectPaintPropertyNode> Clone() const {
    return base::AdoptRef(new EffectPaintPropertyNode(Parent(), State(state_)));
  }

  // The equality operator is used by FindPropertiesNeedingUpdate.h for checking
  // if an effect node has changed.
  bool operator==(const EffectPaintPropertyNode& o) const {
    return Parent() == o.Parent() && state_ == o.state_;
  }
#endif

  std::unique_ptr<JSONObject> ToJSON() const;

  // Returns memory usage of this node plus ancestors.
  size_t TreeMemoryUsageInBytes() const;

 private:
  EffectPaintPropertyNode(scoped_refptr<const EffectPaintPropertyNode> parent,
                          State&& state)
      : PaintPropertyNode(std::move(parent)), state_(std::move(state)) {}

  State state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_EFFECT_PAINT_PROPERTY_NODE_H_
