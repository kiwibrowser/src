// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PAINT_PROPERTY_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PAINT_PROPERTY_NODE_H_

#include "base/optional.h"
#include "third_party/blink/renderer/platform/geometry/float_rounded_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper_clip_cache.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/blink/renderer/platform/graphics/path.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class GeometryMapperClipCache;

// A clip rect created by a css property such as "overflow" or "clip".
// Along with a reference to the transform space the clip rect is based on,
// and a parent ClipPaintPropertyNode for inherited clips.
//
// The clip tree is rooted at a node with no parent. This root node should
// not be modified.
class PLATFORM_EXPORT ClipPaintPropertyNode
    : public PaintPropertyNode<ClipPaintPropertyNode> {
 public:
  // To make it less verbose and more readable to construct and update a node,
  // a struct with default values is used to represent the state.
  struct State {
    scoped_refptr<const TransformPaintPropertyNode> local_transform_space;
    FloatRoundedRect clip_rect;
    base::Optional<FloatRoundedRect> clip_rect_excluding_overlay_scrollbars;
    scoped_refptr<const RefCountedPath> clip_path;
    CompositingReasons direct_compositing_reasons = CompositingReason::kNone;

    // Returns true if the states are equal, ignoring the clip rect excluding
    // overlay scrollbars which is only used for hit testing.
    bool EqualIgnoringHitTestRects(const State& o) const {
      return local_transform_space == o.local_transform_space &&
             clip_rect == o.clip_rect &&
             clip_path == o.clip_path &&
             direct_compositing_reasons == o.direct_compositing_reasons;
    }

    bool operator==(const State& o) const {
      if (!EqualIgnoringHitTestRects(o))
        return false;
      return clip_rect_excluding_overlay_scrollbars ==
             o.clip_rect_excluding_overlay_scrollbars;
    }
  };

  // This node is really a sentinel, and does not represent a real clip space.
  static ClipPaintPropertyNode* Root();

  static scoped_refptr<ClipPaintPropertyNode> Create(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      State&& state) {
    return base::AdoptRef(
        new ClipPaintPropertyNode(std::move(parent), std::move(state)));
  }

  bool Update(scoped_refptr<const ClipPaintPropertyNode> parent,
              State&& state) {
    bool parent_changed = SetParent(parent);
    if (state == state_)
      return parent_changed;

    SetChanged();
    state_ = std::move(state);
    return true;
  }

  bool EqualIgnoringHitTestRects(
      scoped_refptr<const ClipPaintPropertyNode> parent,
      const State& state) const {
    return parent == Parent() && state_.EqualIgnoringHitTestRects(state);
  }

  const TransformPaintPropertyNode* LocalTransformSpace() const {
    return state_.local_transform_space.get();
  }
  const FloatRoundedRect& ClipRect() const { return state_.clip_rect; }
  const FloatRoundedRect& ClipRectExcludingOverlayScrollbars() const {
    return state_.clip_rect_excluding_overlay_scrollbars
               ? *state_.clip_rect_excluding_overlay_scrollbars
               : state_.clip_rect;
  }

  const RefCountedPath* ClipPath() const { return state_.clip_path.get(); }

  bool HasDirectCompositingReasons() const {
    return state_.direct_compositing_reasons != CompositingReason::kNone;
  }

#if DCHECK_IS_ON()
  // The clone function is used by FindPropertiesNeedingUpdate.h for recording
  // a clip node before it has been updated, to later detect changes.
  scoped_refptr<ClipPaintPropertyNode> Clone() const {
    return base::AdoptRef(new ClipPaintPropertyNode(Parent(), State(state_)));
  }

  // The equality operator is used by FindPropertiesNeedingUpdate.h for checking
  // if a clip node has changed.
  bool operator==(const ClipPaintPropertyNode& o) const {
    return Parent() == o.Parent() && state_ == o.state_;
  }
#endif

  std::unique_ptr<JSONObject> ToJSON() const;

  // Returns memory usage of the clip cache of this node plus ancestors.
  size_t CacheMemoryUsageInBytes() const;

 private:
  ClipPaintPropertyNode(scoped_refptr<const ClipPaintPropertyNode> parent,
                        State&& state)
      : PaintPropertyNode(std::move(parent)), state_(std::move(state)) {}

  // For access to GetClipCache();
  friend class GeometryMapper;
  friend class GeometryMapperTest;

  GeometryMapperClipCache& GetClipCache() const {
    return const_cast<ClipPaintPropertyNode*>(this)->GetClipCache();
  }

  GeometryMapperClipCache& GetClipCache() {
    if (!geometry_mapper_clip_cache_)
      geometry_mapper_clip_cache_.reset(new GeometryMapperClipCache());
    return *geometry_mapper_clip_cache_.get();
  }

  State state_;
  std::unique_ptr<GeometryMapperClipCache> geometry_mapper_clip_cache_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_PAINT_PROPERTY_NODE_H_
