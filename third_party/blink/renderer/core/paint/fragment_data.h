// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAGMENT_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAGMENT_DATA_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/platform/graphics/paint/ref_counted_property_tree_state.h"

namespace blink {

class PaintLayer;

// Represents the data for a particular fragment of a LayoutObject.
// Only LayoutObjects with a self-painting PaintLayer may have more than one
// FragmentData, and even then only when they are inside of multicol.
// See README.md.
class CORE_EXPORT FragmentData {
 public:
  FragmentData* NextFragment() const { return next_fragment_.get(); }
  FragmentData& EnsureNextFragment();
  void ClearNextFragment() { next_fragment_.reset(); }

  // Visual offset of this fragment's top-left position from the
  // "paint offset root":
  // - In SPv1 mode, this is the containing composited PaintLayer, or
  //   PaintLayer with a transform, whichever is nearer along the containing
  //   block chain.
  // - In SPv2 mode, this is the containing root PaintLayer of the
  //   root LocalFrameView, or PaintLayer with a transform, whichever is nearer
  //   along the containing block chain.
  LayoutPoint PaintOffset() const { return paint_offset_; }
  void SetPaintOffset(const LayoutPoint& paint_offset) {
    paint_offset_ = paint_offset;
  }

  // The visual rect computed by the latest paint invalidation.
  // This rect does *not* account for composited scrolling. See LayoutObject::
  // AdjustVisualRectForCompositedScrolling().
  LayoutRect VisualRect() const { return visual_rect_; }
  void SetVisualRect(const LayoutRect& rect) { visual_rect_ = rect; }

  // An id for this object that is unique for the lifetime of the WebView.
  UniqueObjectId UniqueId() const {
    return rare_data_ ? rare_data_->unique_id : 0;
  }

  // The PaintLayer associated with this LayoutBoxModelObject. This can be null
  // depending on the return value of LayoutBoxModelObject::LayerTypeRequired().
  PaintLayer* Layer() const {
    return rare_data_ ? rare_data_->layer.get() : nullptr;
  }
  void SetLayer(std::unique_ptr<PaintLayer>);

  // See PaintInvalidatorContext::old_location for details. This will be removed
  // for SPv2.
  LayoutPoint LocationInBacking() const {
    return rare_data_ ? rare_data_->location_in_backing
                      : visual_rect_.Location();
  }
  void SetLocationInBacking(const LayoutPoint& location) {
    if (rare_data_ || location != visual_rect_.Location())
      EnsureRareData().location_in_backing = location;
  }

  // Visual rect of the selection on this object, in the same coordinate space
  // as DisplayItemClient::VisualRect().
  LayoutRect SelectionVisualRect() const {
    return rare_data_ ? rare_data_->selection_visual_rect : LayoutRect();
  }
  void SetSelectionVisualRect(const LayoutRect& r) {
    if (rare_data_ || !r.IsEmpty())
      EnsureRareData().selection_visual_rect = r;
  }

  LayoutRect PartialInvalidationRect() const {
    return rare_data_ ? rare_data_->partial_invalidation_rect : LayoutRect();
  }
  void SetPartialInvalidationRect(const LayoutRect& r) {
    if (rare_data_ || !r.IsEmpty())
      EnsureRareData().partial_invalidation_rect = r;
  }

  LayoutUnit LogicalTopInFlowThread() const {
    return rare_data_ ? rare_data_->logical_top_in_flow_thread : LayoutUnit();
  }
  void SetLogicalTopInFlowThread(LayoutUnit top) {
    if (rare_data_ || top)
      EnsureRareData().logical_top_in_flow_thread = top;
  }

  // The pagination offset is the additional factor to add in to map
  // from flow thread coordinates relative to the enclosing pagination
  // layer, to visual coordiantes relative to that pagination layer.
  LayoutPoint PaginationOffset() const {
    return rare_data_ ? rare_data_->pagination_offset : LayoutPoint();
  }
  void SetPaginationOffset(const LayoutPoint& pagination_offset) {
    if (rare_data_ || pagination_offset != LayoutPoint())
      EnsureRareData().pagination_offset = pagination_offset;
  }

  bool IsClipPathCacheValid() const {
    return rare_data_ && rare_data_->is_clip_path_cache_valid;
  }
  void InvalidateClipPathCache();

  base::Optional<IntRect> ClipPathBoundingBox() const {
    DCHECK(IsClipPathCacheValid());
    return rare_data_ ? rare_data_->clip_path_bounding_box : base::nullopt;
  }
  const RefCountedPath* ClipPathPath() const {
    DCHECK(IsClipPathCacheValid());
    return rare_data_ ? rare_data_->clip_path_path.get() : nullptr;
  }
  void SetClipPathCache(const base::Optional<IntRect>& bounding_box,
                        scoped_refptr<const RefCountedPath>);

  // Holds references to the paint property nodes created by this object.
  const ObjectPaintProperties* PaintProperties() const {
    return rare_data_ ? rare_data_->paint_properties.get() : nullptr;
  }
  ObjectPaintProperties* PaintProperties() {
    return rare_data_ ? rare_data_->paint_properties.get() : nullptr;
  }
  ObjectPaintProperties& EnsurePaintProperties() {
    EnsureRareData();
    if (!rare_data_->paint_properties)
      rare_data_->paint_properties = ObjectPaintProperties::Create();
    return *rare_data_->paint_properties;
  }
  void ClearPaintProperties() {
    if (rare_data_)
      rare_data_->paint_properties = nullptr;
  }

  // This is a complete set of property nodes that should be used as a
  // starting point to paint a LayoutObject. This data is cached because some
  // properties inherit from the containing block chain instead of the
  // painting parent and cannot be derived in O(1) during the paint walk.
  // LocalBorderBoxProperties() includes fragment clip.
  //
  // For example: <div style='opacity: 0.3;'/>
  //   The div's local border box properties would have an opacity 0.3 effect
  //   node. Even though the div has no transform, its local border box
  //   properties would have a transform node that points to the div's
  //   ancestor transform space.
  PropertyTreeState LocalBorderBoxProperties() const {
    DCHECK(HasLocalBorderBoxProperties());
    return rare_data_->local_border_box_properties->GetPropertyTreeState();
  }
  bool HasLocalBorderBoxProperties() const {
    return rare_data_ && rare_data_->local_border_box_properties;
  }
  void ClearLocalBorderBoxProperties() {
    if (rare_data_)
      rare_data_->local_border_box_properties = nullptr;
  }
  void SetLocalBorderBoxProperties(const PropertyTreeState& state) {
    EnsureRareData();
    if (!rare_data_->local_border_box_properties) {
      rare_data_->local_border_box_properties =
          std::make_unique<RefCountedPropertyTreeState>(state);
    } else {
      *rare_data_->local_border_box_properties = std::move(state);
    }
  }

  // This is the complete set of property nodes that is inherited
  // from the ancestor before applying any local CSS properties,
  // but includes paint offset transform.
  PropertyTreeState PreEffectProperties() const {
    return PropertyTreeState(PreTransform(), PreClip(), PreEffect());
  }

  // This is the complete set of property nodes that can be used to
  // paint the contents of this fragment. It is similar to
  // |local_border_box_properties_| but includes properties (e.g.,
  // overflow clip, scroll translation) that apply to contents.
  PropertyTreeState ContentsProperties() const {
    return PropertyTreeState(PostScrollTranslation(), PostOverflowClip(),
                             LocalBorderBoxProperties().Effect());
  }

  // This is the complete set of property nodes that can be used to
  // paint mask-based clip-path.
  PropertyTreeState ClipPathProperties() const {
    DCHECK(rare_data_);
    const auto* properties = rare_data_->paint_properties.get();
    DCHECK(properties);
    DCHECK(properties->MaskClip());
    DCHECK(properties->ClipPath());
    return PropertyTreeState(properties->MaskClip()->LocalTransformSpace(),
                             properties->MaskClip(), properties->ClipPath());
  }

  const TransformPaintPropertyNode* PreTransform() const;
  const TransformPaintPropertyNode* PostScrollTranslation() const;
  const ClipPaintPropertyNode* PreClip() const;
  const ClipPaintPropertyNode* PostOverflowClip() const;
  const EffectPaintPropertyNode* PreEffect() const;
  const EffectPaintPropertyNode* PreFilter() const;

 private:
  friend class FragmentDataTest;

  // Contains rare data that that is not needed on all fragments.
  struct RareData {
    USING_FAST_MALLOC(RareData);

   public:
    RareData(const LayoutPoint& location_in_backing);
    ~RareData();

    // The following data fields are not fragment specific. Placed here just to
    // avoid separate data structure for them.
    std::unique_ptr<PaintLayer> layer;
    UniqueObjectId unique_id;
    LayoutPoint location_in_backing;
    LayoutRect selection_visual_rect;
    LayoutRect partial_invalidation_rect;

    // Fragment specific data.
    LayoutPoint pagination_offset;
    LayoutUnit logical_top_in_flow_thread;
    std::unique_ptr<ObjectPaintProperties> paint_properties;
    std::unique_ptr<RefCountedPropertyTreeState> local_border_box_properties;
    bool is_clip_path_cache_valid = false;
    base::Optional<IntRect> clip_path_bounding_box;
    scoped_refptr<const RefCountedPath> clip_path_path;

    DISALLOW_COPY_AND_ASSIGN(RareData);
  };

  RareData& EnsureRareData();

  LayoutRect visual_rect_;
  LayoutPoint paint_offset_;

  std::unique_ptr<RareData> rare_data_;
  std::unique_ptr<FragmentData> next_fragment_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FRAGMENT_DATA_H_
