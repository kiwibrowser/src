// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_LAYER_H_
#define CC_LAYERS_LAYER_H_

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "cc/base/region.h"
#include "cc/benchmarks/micro_benchmark.h"
#include "cc/cc_export.h"
#include "cc/input/input_handler.h"
#include "cc/input/overscroll_behavior.h"
#include "cc/input/scroll_snap_data.h"
#include "cc/layers/layer_collections.h"
#include "cc/layers/layer_position_constraint.h"
#include "cc/layers/touch_action_region.h"
#include "cc/paint/filter_operations.h"
#include "cc/paint/paint_record.h"
#include "cc/trees/element_id.h"
#include "cc/trees/mutator_host_client.h"
#include "cc/trees/property_tree.h"
#include "cc/trees/target_property.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/transform.h"

namespace base {
namespace trace_event {
class TracedValue;
}
}

namespace viz {
class CopyOutputRequest;
}

namespace cc {

class LayerClient;
class LayerImpl;
class LayerTreeHost;
class LayerTreeHostCommon;
class LayerTreeImpl;
class MutatorHost;
class ScrollbarLayerInterface;

// Base class for composited layers. Special layer types are derived from
// this class.
class CC_EXPORT Layer : public base::RefCounted<Layer> {
 public:
  using LayerListType = LayerList;

  enum LayerIdLabels {
    INVALID_ID = -1,
  };

  enum LayerMaskType {
    NOT_MASK = 0,
    MULTI_TEXTURE_MASK,
    SINGLE_TEXTURE_MASK,
  };

  static scoped_refptr<Layer> Create();

  int id() const { return inputs_.layer_id; }

  Layer* RootLayer();
  Layer* parent() { return parent_; }
  const Layer* parent() const { return parent_; }
  void AddChild(scoped_refptr<Layer> child);
  void InsertChild(scoped_refptr<Layer> child, size_t index);
  void ReplaceChild(Layer* reference, scoped_refptr<Layer> new_layer);
  void RemoveFromParent();
  void RemoveAllChildren();
  void SetChildren(const LayerList& children);
  bool HasAncestor(const Layer* ancestor) const;

  const LayerList& children() const { return inputs_.children; }
  Layer* child_at(size_t index) { return inputs_.children[index].get(); }

  // This requests the layer and its subtree be rendered and given to the
  // callback. If the copy is unable to be produced (the layer is destroyed
  // first), then the callback is called with a nullptr/empty result. If the
  // request's source property is set, any prior uncommitted requests having the
  // same source will be aborted.
  void RequestCopyOfOutput(std::unique_ptr<viz::CopyOutputRequest> request);
  bool HasCopyRequest() const { return !inputs_.copy_requests.empty(); }

  void SetSubtreeHasCopyRequest(bool subtree_has_copy_request);
  bool SubtreeHasCopyRequest() const;

  void TakeCopyRequests(
      std::vector<std::unique_ptr<viz::CopyOutputRequest>>* requests);

  virtual void SetBackgroundColor(SkColor background_color);
  SkColor background_color() const { return inputs_.background_color; }
  void SetSafeOpaqueBackgroundColor(SkColor background_color);
  // If contents_opaque(), return an opaque color else return a
  // non-opaque color.  Tries to return background_color(), if possible.
  SkColor SafeOpaqueBackgroundColor() const;

  // A layer's bounds are in logical, non-page-scaled pixels (however, the
  // root layer's bounds are in physical pixels).
  void SetBounds(const gfx::Size& bounds);
  const gfx::Size& bounds() const { return inputs_.bounds; }

  void SetOverscrollBehavior(const OverscrollBehavior& behavior);
  OverscrollBehavior overscroll_behavior() const {
    return inputs_.overscroll_behavior;
  }

  void SetSnapContainerData(base::Optional<SnapContainerData> data);
  const base::Optional<SnapContainerData>& snap_container_data() const {
    return inputs_.snap_container_data;
  }

  void SetMasksToBounds(bool masks_to_bounds);
  bool masks_to_bounds() const { return inputs_.masks_to_bounds; }

  void SetMaskLayer(Layer* mask_layer);
  Layer* mask_layer() { return inputs_.mask_layer.get(); }
  const Layer* mask_layer() const { return inputs_.mask_layer.get(); }

  // Marks the |dirty_rect| as being changed, which will cause a commit and
  // the compositor to submit a new frame with a damage rect that includes the
  // layer's dirty area.
  virtual void SetNeedsDisplayRect(const gfx::Rect& dirty_rect);
  // Marks the entire layer's bounds as being changed, which will cause a commit
  // and the compositor to submit a new frame with a damage rect that includes
  // the entire layer.
  void SetNeedsDisplay() { SetNeedsDisplayRect(gfx::Rect(bounds())); }

  virtual void SetOpacity(float opacity);
  float opacity() const { return inputs_.opacity; }
  float EffectiveOpacity() const;
  virtual bool OpacityCanAnimateOnImplThread() const;

  void SetBlendMode(SkBlendMode blend_mode);
  SkBlendMode blend_mode() const { return inputs_.blend_mode; }

  // A layer is root for an isolated group when it and all its descendants are
  // drawn over a black and fully transparent background, creating an isolated
  // group. It should be used along with SetBlendMode(), in order to restrict
  // layers within the group to blend with layers outside this group.
  void SetIsRootForIsolatedGroup(bool root);
  bool is_root_for_isolated_group() const {
    return inputs_.is_root_for_isolated_group;
  }

  // Make the layer hit testable even if |draws_content_| is false.
  void SetHitTestableWithoutDrawsContent(bool should_hit_test);
  bool hit_testable_without_draws_content() const {
    return inputs_.hit_testable_without_draws_content;
  }

  void SetFilters(const FilterOperations& filters);
  const FilterOperations& filters() const { return inputs_.filters; }

  // Background filters are filters applied to what is behind this layer, when
  // they are viewed through non-opaque regions in this layer.
  void SetBackgroundFilters(const FilterOperations& filters);
  const FilterOperations& background_filters() const {
    return inputs_.background_filters;
  }

  void SetContentsOpaque(bool opaque);
  bool contents_opaque() const { return inputs_.contents_opaque; }

  void SetPosition(const gfx::PointF& position);
  const gfx::PointF& position() const { return inputs_.position; }

  // A layer that is a container for fixed position layers cannot be both
  // scrollable and have a non-identity transform.
  void SetIsContainerForFixedPositionLayers(bool container);
  bool IsContainerForFixedPositionLayers() const;

  void SetIsResizedByBrowserControls(bool resized);
  bool IsResizedByBrowserControls() const;

  void SetPositionConstraint(const LayerPositionConstraint& constraint);
  const LayerPositionConstraint& position_constraint() const {
    return inputs_.position_constraint;
  }

  void SetStickyPositionConstraint(
      const LayerStickyPositionConstraint& constraint);
  const LayerStickyPositionConstraint& sticky_position_constraint() const {
    return inputs_.sticky_position_constraint;
  }

  TransformNode* GetTransformNode() const;

  void SetTransform(const gfx::Transform& transform);
  const gfx::Transform& transform() const { return inputs_.transform; }

  void SetTransformOrigin(const gfx::Point3F&);
  const gfx::Point3F& transform_origin() const {
    return inputs_.transform_origin;
  }

  void SetScrollParent(Layer* parent);

  Layer* scroll_parent() { return inputs_.scroll_parent; }

  void SetClipParent(Layer* ancestor);

  Layer* clip_parent() { return inputs_.clip_parent; }

  std::set<Layer*>* clip_children() { return clip_children_.get(); }
  const std::set<Layer*>* clip_children() const {
    return clip_children_.get();
  }

  gfx::Transform ScreenSpaceTransform() const;

  void set_num_unclipped_descendants(size_t descendants) {
    num_unclipped_descendants_ = descendants;
  }
  size_t num_unclipped_descendants() const {
    return num_unclipped_descendants_;
  }

  void SetScrollOffset(const gfx::ScrollOffset& scroll_offset);

  const gfx::ScrollOffset& scroll_offset() const {
    return inputs_.scroll_offset;
  }
  // Called internally during commit to update the layer with state from the
  // compositor thread. Not to be called externally by users of this class.
  void SetScrollOffsetFromImplSide(const gfx::ScrollOffset& scroll_offset);

  // Marks this layer as being scrollable and needing an associated scroll node.
  // The scroll node's bounds and container_bounds will be kept in sync
  // with this layer. Once scrollable, a Layer cannot become un-scrollable.
  void SetScrollable(const gfx::Size& scroll_container_bounds);
  const gfx::Size& scroll_container_bounds() const {
    return inputs_.scroll_container_bounds;
  }
  bool scrollable() const { return inputs_.scrollable; }

  void SetUserScrollable(bool horizontal, bool vertical);
  bool user_scrollable_horizontal() const {
    return inputs_.user_scrollable_horizontal;
  }
  bool user_scrollable_vertical() const {
    return inputs_.user_scrollable_vertical;
  }

  void AddMainThreadScrollingReasons(uint32_t main_thread_scrolling_reasons);
  void ClearMainThreadScrollingReasons(
      uint32_t main_thread_scrolling_reasons_to_clear);
  uint32_t main_thread_scrolling_reasons() const {
    return inputs_.main_thread_scrolling_reasons;
  }
  bool should_scroll_on_main_thread() const {
    return !!inputs_.main_thread_scrolling_reasons;
  }

  void SetNonFastScrollableRegion(const Region& non_fast_scrollable_region);
  const Region& non_fast_scrollable_region() const {
    return inputs_.non_fast_scrollable_region;
  }

  void SetTouchActionRegion(TouchActionRegion touch_action_region);
  const TouchActionRegion& touch_action_region() const {
    return inputs_.touch_action_region;
  }

  void set_did_scroll_callback(
      base::Callback<void(const gfx::ScrollOffset&, const ElementId&)>
          callback) {
    inputs_.did_scroll_callback = std::move(callback);
  }

  void SetCacheRenderSurface(bool cache_render_surface);
  bool cache_render_surface() const { return cache_render_surface_; }

  void SetForceRenderSurfaceForTesting(bool force_render_surface);
  bool force_render_surface_for_testing() const {
    return force_render_surface_for_testing_;
  }

  const gfx::ScrollOffset& CurrentScrollOffset() const {
    return inputs_.scroll_offset;
  }

  void SetDoubleSided(bool double_sided);
  bool double_sided() const { return inputs_.double_sided; }

  void SetShouldFlattenTransform(bool flatten);
  bool should_flatten_transform() const {
    return inputs_.should_flatten_transform;
  }

  bool Is3dSorted() const { return inputs_.sorting_context_id != 0; }

  void SetUseParentBackfaceVisibility(bool use);
  bool use_parent_backface_visibility() const {
    return inputs_.use_parent_backface_visibility;
  }

  void SetShouldCheckBackfaceVisibility(bool should_check_backface_visibility);
  bool should_check_backface_visibility() const {
    return should_check_backface_visibility_;
  }

  virtual void SetLayerTreeHost(LayerTreeHost* host);

  // When true the layer may contribute to the compositor's output. When false,
  // it does not. This property does not apply to children of the layer, they
  // may contribute while this layer does not. The layer itself will determine
  // if it has content to contribute, but when false, this prevents it from
  // doing so.
  void SetIsDrawable(bool is_drawable);

  void SetHideLayerAndSubtree(bool hide);
  bool hide_layer_and_subtree() const { return inputs_.hide_layer_and_subtree; }

  void SetFiltersOrigin(const gfx::PointF& origin);
  gfx::PointF filters_origin() const { return inputs_.filters_origin; }

  int NumDescendantsThatDrawContent() const;

  // Is true if the layer will contribute content to the compositor's output.
  // Will be false if SetIsDrawable(false) is called. But will also be false if
  // the layer itself has no content to contribute, even though the layer was
  // given SetIsDrawable(true).
  // This is only virtual for tests.
  // TODO(awoloszyn): Remove this once we no longer need it for tests
  virtual bool DrawsContent() const;

  // This methods typically need to be overwritten by derived classes.
  // Returns true iff anything was updated that needs to be committed.
  virtual bool Update();
  virtual void SetLayerMaskType(Layer::LayerMaskType type) {}
  virtual bool HasSlowPaths() const;
  virtual bool HasNonAAPaint() const;

  void UpdateDebugInfo();

  void SetLayerClient(base::WeakPtr<LayerClient> client);
  LayerClient* GetLayerClientForTesting() const { return inputs_.client.get(); }

  virtual bool IsSnapped();

  virtual void PushPropertiesTo(LayerImpl* layer);

  LayerTreeHost* GetLayerTreeHostForTesting() const { return layer_tree_host_; }

  virtual ScrollbarLayerInterface* ToScrollbarLayer();

  virtual sk_sp<SkPicture> GetPicture() const;

  // Constructs a LayerImpl of the correct runtime type for this Layer type.
  virtual std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl);

  bool NeedsDisplayForTesting() const { return !inputs_.update_rect.IsEmpty(); }
  void ResetNeedsDisplayForTesting() { inputs_.update_rect = gfx::Rect(); }

  // Mark the layer as needing to push its properties to the LayerImpl during
  // commit.
  void SetNeedsPushProperties();
  void ResetNeedsPushPropertiesForTesting();

  virtual void RunMicroBenchmark(MicroBenchmark* benchmark);

  void Set3dSortingContextId(int id);
  int sorting_context_id() const { return inputs_.sorting_context_id; }

  void set_property_tree_sequence_number(int sequence_number) {
    property_tree_sequence_number_ = sequence_number;
  }
  int property_tree_sequence_number() const {
    return property_tree_sequence_number_;
  }

  void SetTransformTreeIndex(int index);
  int transform_tree_index() const;

  void SetClipTreeIndex(int index);
  int clip_tree_index() const;

  void SetEffectTreeIndex(int index);
  int effect_tree_index() const;

  void SetScrollTreeIndex(int index);
  int scroll_tree_index() const;

  void set_offset_to_transform_parent(gfx::Vector2dF offset) {
    if (offset_to_transform_parent_ == offset)
      return;
    offset_to_transform_parent_ = offset;
    SetNeedsPushProperties();
  }
  gfx::Vector2dF offset_to_transform_parent() const {
    return offset_to_transform_parent_;
  }

  // TODO(enne): This needs a different name.  It is a calculated value
  // from the property tree builder and not a synonym for "should
  // flatten transform".
  void set_should_flatten_transform_from_property_tree(bool should_flatten) {
    if (should_flatten_transform_from_property_tree_ == should_flatten)
      return;
    should_flatten_transform_from_property_tree_ = should_flatten;
    SetNeedsPushProperties();
  }
  bool should_flatten_transform_from_property_tree() const {
    return should_flatten_transform_from_property_tree_;
  }

  const gfx::Rect& visible_layer_rect_for_testing() const {
    return visible_layer_rect_;
  }
  void set_visible_layer_rect(const gfx::Rect& rect) {
    visible_layer_rect_ = rect;
  }

  // This is for tracking damage.
  void SetSubtreePropertyChanged();
  bool subtree_property_changed() const { return subtree_property_changed_; }

  void SetMayContainVideo(bool yes);

  bool has_copy_requests_in_target_subtree();

  // Stable identifier for clients. See comment in cc/trees/element_id.h.
  void SetElementId(ElementId id);
  ElementId element_id() const { return inputs_.element_id; }

  bool HasTickingAnimationForTesting() const;

  void SetHasWillChangeTransformHint(bool has_will_change);
  bool has_will_change_transform_hint() const {
    return inputs_.has_will_change_transform_hint;
  }

  void SetTrilinearFiltering(bool trilinear_filtering);
  bool trilinear_filtering() const { return inputs_.trilinear_filtering; }

  MutatorHost* GetMutatorHost() const;

  ElementListType GetElementTypeForAnimation() const;

  void SetScrollbarsHiddenFromImplSide(bool hidden);

  const gfx::Rect& update_rect() const { return inputs_.update_rect; }

  LayerTreeHost* layer_tree_host() const { return layer_tree_host_; }

  // Called on the scroll layer to trigger showing the overlay scrollbars.
  void ShowScrollbars() { needs_show_scrollbars_ = true; }

  bool has_transform_node() { return has_transform_node_; }
  void SetHasTransformNode(bool val) { has_transform_node_ = val; }

 protected:
  friend class LayerImpl;
  friend class TreeSynchronizer;
  virtual ~Layer();
  Layer();

  // These SetNeeds functions are in order of severity of update:
  //
  // Called when a property has been modified in a way that the layer knows
  // immediately that a commit is required.  This implies SetNeedsPushProperties
  // to push that property.
  void SetNeedsCommit();

  // Called when there's been a change in layer structure.  Implies
  // SetNeedsCommit and property tree rebuld, but not SetNeedsPushProperties
  // (the full tree is synced over).
  void SetNeedsFullTreeSync();

  // Called when the next commit should wait until the pending tree is activated
  // before finishing the commit and unblocking the main thread. Used to ensure
  // unused resources on the impl thread are returned before commit completes.
  void SetNextCommitWaitsForActivation();

  // Will recalculate whether the layer draws content and set draws_content_
  // appropriately.
  void UpdateDrawsContent(bool has_drawable_content);
  virtual bool HasDrawableContent() const;

  // Called when the layer's number of drawable descendants changes.
  void AddDrawableDescendants(int num);

  bool IsPropertyChangeAllowed() const;

  // When true, the layer is about to perform an update. Any commit requests
  // will be handled implicitly after the update completes.
  bool ignore_set_needs_commit_;

 private:
  friend class base::RefCounted<Layer>;
  friend class LayerTreeHostCommon;
  friend class LayerTreeHost;

  // Interactions with attached animations.
  void OnFilterAnimated(const FilterOperations& filters);
  void OnOpacityAnimated(float opacity);
  void OnTransformAnimated(const gfx::Transform& transform);

  bool ScrollOffsetAnimationWasInterrupted() const;

  void AddClipChild(Layer* child);
  void RemoveClipChild(Layer* child);

  void SetParent(Layer* layer);
  bool DescendantIsFixedToContainerLayer() const;

  // This should only be called from RemoveFromParent().
  void RemoveChildOrDependent(Layer* child);

  // If this layer has a clip parent, it removes |this| from its list of clip
  // children.
  void RemoveFromClipTree();

  // When we detach or attach layer to new LayerTreeHost, all property trees'
  // indices becomes invalid.
  void InvalidatePropertyTreesIndices();

  // This is set whenever a property changed on layer that affects whether this
  // layer should own a property tree node or not.
  void SetPropertyTreesNeedRebuild();

  // Fast-path for |SetScrollOffset| and |SetScrollOffsetFromImplSide| to
  // directly update scroll offset values in the property tree without needing a
  // full property tree update. If property trees do not exist yet, ensures
  // they are marked as needing to be rebuilt.
  void UpdateScrollOffset(const gfx::ScrollOffset&);

  // Encapsulates all data, callbacks or interfaces received from the embedder.
  // TODO(khushalsagar): This is only valid when PropertyTrees are built
  // internally in cc. Update this for the SPv2 path where blink generates
  // PropertyTrees.
  struct Inputs {
    explicit Inputs(int layer_id);
    ~Inputs();

    int layer_id;

    LayerList children;

    // The update rect is the region of the compositor resource that was
    // actually updated by the compositor. For layers that may do updating
    // outside the compositor's control (i.e. plugin layers), this information
    // is not available and the update rect will remain empty.
    // Note this rect is in layer space (not content space).
    gfx::Rect update_rect;

    gfx::Size bounds;
    bool masks_to_bounds;

    scoped_refptr<Layer> mask_layer;

    float opacity;
    SkBlendMode blend_mode;

    bool is_root_for_isolated_group : 1;

    // Hit testing depends on draws_content (see: |LayerImpl::should_hit_test|)
    // and this bit can be set to cause the LayerImpl to be hit testable without
    // draws_content.
    bool hit_testable_without_draws_content : 1;

    bool contents_opaque : 1;

    gfx::PointF position;
    gfx::Transform transform;
    gfx::Point3F transform_origin;

    bool is_drawable : 1;

    bool double_sided : 1;
    bool should_flatten_transform : 1;

    // Layers that share a sorting context id will be sorted together in 3d
    // space.  0 is a special value that means this layer will not be sorted
    // and will be drawn in paint order.
    int sorting_context_id;

    bool use_parent_backface_visibility : 1;

    SkColor background_color;

    FilterOperations filters;
    FilterOperations background_filters;
    gfx::PointF filters_origin;

    gfx::ScrollOffset scroll_offset;

    // Size of the scroll container that this layer scrolls in.
    gfx::Size scroll_container_bounds;

    // Indicates that this layer will need a scroll property node and that this
    // layer's bounds correspond to the scroll node's bounds (both |bounds| and
    // |scroll_container_bounds|).
    bool scrollable : 1;

    bool user_scrollable_horizontal : 1;
    bool user_scrollable_vertical : 1;

    uint32_t main_thread_scrolling_reasons;
    Region non_fast_scrollable_region;

    TouchActionRegion touch_action_region;

    // When set, position: fixed children of this layer will be affected by URL
    // bar movement. bottom-fixed element will be pushed down as the URL bar
    // hides (and the viewport expands) so that the element stays fixed to the
    // viewport bottom. This will always be set on the outer viewport scroll
    // layer. In the case of a non-default rootScroller, all iframes in the
    // rootScroller ancestor chain will also have it set on their scroll
    // layers.
    bool is_resized_by_browser_controls : 1;
    bool is_container_for_fixed_position_layers : 1;
    LayerPositionConstraint position_constraint;

    LayerStickyPositionConstraint sticky_position_constraint;

    ElementId element_id;

    Layer* scroll_parent;
    Layer* clip_parent;

    bool has_will_change_transform_hint : 1;

    bool trilinear_filtering : 1;

    bool hide_layer_and_subtree : 1;

    // The following elements can not and are not serialized.
    base::WeakPtr<LayerClient> client;
    std::unique_ptr<base::trace_event::TracedValue> debug_info;

    base::Callback<void(const gfx::ScrollOffset&, const ElementId&)>
        did_scroll_callback;
    std::vector<std::unique_ptr<viz::CopyOutputRequest>> copy_requests;

    OverscrollBehavior overscroll_behavior;

    base::Optional<SnapContainerData> snap_container_data;
  };

  Layer* parent_;

  // Layer instances have a weak pointer to their LayerTreeHost.
  // This pointer value is nil when a Layer is not in a tree and is
  // updated via SetLayerTreeHost() if a layer moves between trees.
  LayerTreeHost* layer_tree_host_;

  Inputs inputs_;

  int num_descendants_that_draw_content_;
  int transform_tree_index_;
  int effect_tree_index_;
  int clip_tree_index_;
  int scroll_tree_index_;
  int property_tree_sequence_number_;
  gfx::Vector2dF offset_to_transform_parent_;
  bool should_flatten_transform_from_property_tree_ : 1;
  bool draws_content_ : 1;
  bool should_check_backface_visibility_ : 1;
  // Force use of and cache render surface.
  bool cache_render_surface_ : 1;
  bool force_render_surface_for_testing_ : 1;
  bool subtree_property_changed_ : 1;
  bool may_contain_video_ : 1;
  bool needs_show_scrollbars_ : 1;
  // Whether the nodes referred to by *_tree_index_ "belong" to this
  // layer. Only applicable if LayerTreeSettings.use_layer_lists is
  // false.
  bool has_transform_node_ : 1;
  // This value is valid only when LayerTreeHost::has_copy_request() is true
  bool subtree_has_copy_request_ : 1;
  SkColor safe_opaque_background_color_;

  std::unique_ptr<std::set<Layer*>> clip_children_;

  // These all act like draw properties, so don't need push properties.
  gfx::Rect visible_layer_rect_;
  size_t num_unclipped_descendants_;

  DISALLOW_COPY_AND_ASSIGN(Layer);
};

}  // namespace cc

#endif  // CC_LAYERS_LAYER_H_
