// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/layer.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include "base/atomic_sequence_num.h"
#include "base/location.h"
#include "base/metrics/histogram.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "cc/base/simple_enclosed_region.h"
#include "cc/input/main_thread_scrolling_reason.h"
#include "cc/layers/layer_client.h"
#include "cc/layers/layer_impl.h"
#include "cc/layers/scrollbar_layer_interface.h"
#include "cc/tiles/frame_viewer_instrumentation.h"
#include "cc/trees/draw_property_utils.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/mutator_host.h"
#include "cc/trees/scroll_node.h"
#include "cc/trees/transform_node.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/vector2d_conversions.h"

namespace cc {

base::AtomicSequenceNumber g_next_layer_id;

Layer::Inputs::Inputs(int layer_id)
    : layer_id(layer_id),
      masks_to_bounds(false),
      mask_layer(nullptr),
      opacity(1.f),
      blend_mode(SkBlendMode::kSrcOver),
      is_root_for_isolated_group(false),
      hit_testable_without_draws_content(false),
      contents_opaque(false),
      is_drawable(false),
      double_sided(true),
      should_flatten_transform(true),
      sorting_context_id(0),
      use_parent_backface_visibility(false),
      background_color(0),
      scrollable(false),
      user_scrollable_horizontal(true),
      user_scrollable_vertical(true),
      main_thread_scrolling_reasons(
          MainThreadScrollingReason::kNotScrollingOnMain),
      is_resized_by_browser_controls(false),
      is_container_for_fixed_position_layers(false),
      scroll_parent(nullptr),
      clip_parent(nullptr),
      has_will_change_transform_hint(false),
      trilinear_filtering(false),
      hide_layer_and_subtree(false),
      overscroll_behavior(OverscrollBehavior::kOverscrollBehaviorTypeAuto) {}

Layer::Inputs::~Inputs() = default;

scoped_refptr<Layer> Layer::Create() {
  return base::WrapRefCounted(new Layer());
}

Layer::Layer()
    : ignore_set_needs_commit_(false),
      parent_(nullptr),
      layer_tree_host_(nullptr),
      // Layer IDs start from 1.
      inputs_(g_next_layer_id.GetNext() + 1),
      num_descendants_that_draw_content_(0),
      transform_tree_index_(TransformTree::kInvalidNodeId),
      effect_tree_index_(EffectTree::kInvalidNodeId),
      clip_tree_index_(ClipTree::kInvalidNodeId),
      scroll_tree_index_(ScrollTree::kInvalidNodeId),
      property_tree_sequence_number_(-1),
      should_flatten_transform_from_property_tree_(false),
      draws_content_(false),
      should_check_backface_visibility_(false),
      cache_render_surface_(false),
      force_render_surface_for_testing_(false),
      subtree_property_changed_(false),
      may_contain_video_(false),
      needs_show_scrollbars_(false),
      has_transform_node_(false),
      subtree_has_copy_request_(false),
      safe_opaque_background_color_(0),
      num_unclipped_descendants_(0) {}

Layer::~Layer() {
  // Our parent should be holding a reference to us so there should be no
  // way for us to be destroyed while we still have a parent.
  DCHECK(!parent());
  // Similarly we shouldn't have a layer tree host since it also keeps a
  // reference to us.
  DCHECK(!layer_tree_host());

  RemoveFromClipTree();

  // Remove the parent reference from all children and dependents.
  RemoveAllChildren();
  if (inputs_.mask_layer.get()) {
    DCHECK_EQ(this, inputs_.mask_layer->parent());
    inputs_.mask_layer->RemoveFromParent();
  }
}

void Layer::SetLayerTreeHost(LayerTreeHost* host) {
  if (layer_tree_host_ == host)
    return;

  if (layer_tree_host_) {
    layer_tree_host_->property_trees()->needs_rebuild = true;
    layer_tree_host_->UnregisterLayer(this);
    if (!layer_tree_host_->IsUsingLayerLists() && inputs_.element_id) {
      layer_tree_host_->UnregisterElement(inputs_.element_id,
                                          ElementListType::ACTIVE);
    }
  }
  if (host) {
    host->property_trees()->needs_rebuild = true;
    host->RegisterLayer(this);
    if (!host->IsUsingLayerLists() && inputs_.element_id) {
      host->RegisterElement(inputs_.element_id, ElementListType::ACTIVE, this);
    }
  }

  layer_tree_host_ = host;
  InvalidatePropertyTreesIndices();

  // When changing hosts, the layer needs to commit its properties to the impl
  // side for the new host.
  SetNeedsPushProperties();

  for (size_t i = 0; i < inputs_.children.size(); ++i)
    inputs_.children[i]->SetLayerTreeHost(host);

  if (inputs_.mask_layer.get())
    inputs_.mask_layer->SetLayerTreeHost(host);

  if (host && !host->IsUsingLayerLists() &&
      GetMutatorHost()->IsElementAnimating(element_id())) {
    host->SetNeedsCommit();
  }
}

void Layer::SetNeedsCommit() {
  if (!layer_tree_host_)
    return;

  SetNeedsPushProperties();

  if (ignore_set_needs_commit_)
    return;

  layer_tree_host_->SetNeedsCommit();
}

void Layer::SetNeedsFullTreeSync() {
  if (!layer_tree_host_)
    return;

  layer_tree_host_->SetNeedsFullTreeSync();
}

void Layer::SetNextCommitWaitsForActivation() {
  if (!layer_tree_host_)
    return;

  layer_tree_host_->SetNextCommitWaitsForActivation();
}

void Layer::SetNeedsPushProperties() {
  if (layer_tree_host_)
    layer_tree_host_->AddLayerShouldPushProperties(this);
}

void Layer::ResetNeedsPushPropertiesForTesting() {
  if (layer_tree_host_)
    layer_tree_host_->RemoveLayerShouldPushProperties(this);
}

bool Layer::IsPropertyChangeAllowed() const {
  if (!layer_tree_host_)
    return true;

  return !layer_tree_host_->in_paint_layer_contents();
}

sk_sp<SkPicture> Layer::GetPicture() const {
  return nullptr;
}

void Layer::SetParent(Layer* layer) {
  DCHECK(!layer || !layer->HasAncestor(this));

  parent_ = layer;
  SetLayerTreeHost(parent_ ? parent_->layer_tree_host() : nullptr);

  SetPropertyTreesNeedRebuild();
}

void Layer::AddChild(scoped_refptr<Layer> child) {
  InsertChild(child, inputs_.children.size());
}

void Layer::InsertChild(scoped_refptr<Layer> child, size_t index) {
  DCHECK(IsPropertyChangeAllowed());
  child->RemoveFromParent();
  AddDrawableDescendants(child->NumDescendantsThatDrawContent() +
                         (child->DrawsContent() ? 1 : 0));
  child->SetParent(this);
  child->SetSubtreePropertyChanged();

  index = std::min(index, inputs_.children.size());
  inputs_.children.insert(inputs_.children.begin() + index, child);
  SetNeedsFullTreeSync();
}

void Layer::RemoveFromParent() {
  DCHECK(IsPropertyChangeAllowed());
  if (parent_)
    parent_->RemoveChildOrDependent(this);
}

void Layer::RemoveChildOrDependent(Layer* child) {
  if (inputs_.mask_layer.get() == child) {
    inputs_.mask_layer->SetParent(nullptr);
    inputs_.mask_layer = nullptr;
    SetNeedsFullTreeSync();
    return;
  }

  for (LayerList::iterator iter = inputs_.children.begin();
       iter != inputs_.children.end(); ++iter) {
    if (iter->get() != child)
      continue;

    child->SetParent(nullptr);
    AddDrawableDescendants(-child->NumDescendantsThatDrawContent() -
                           (child->DrawsContent() ? 1 : 0));
    inputs_.children.erase(iter);
    SetNeedsFullTreeSync();
    return;
  }
}

void Layer::ReplaceChild(Layer* reference, scoped_refptr<Layer> new_layer) {
  DCHECK(reference);
  DCHECK_EQ(reference->parent(), this);
  DCHECK(IsPropertyChangeAllowed());

  if (reference == new_layer.get())
    return;

  // Find the index of |reference| in |children_|.
  auto reference_it =
      std::find_if(inputs_.children.begin(), inputs_.children.end(),
                   [reference](const scoped_refptr<Layer>& layer) {
                     return layer.get() == reference;
                   });
  DCHECK(reference_it != inputs_.children.end());
  size_t reference_index = reference_it - inputs_.children.begin();
  reference->RemoveFromParent();

  if (new_layer.get()) {
    new_layer->RemoveFromParent();
    InsertChild(new_layer, reference_index);
  }
}

void Layer::SetBounds(const gfx::Size& size) {
  DCHECK(IsPropertyChangeAllowed());
  if (bounds() == size)
    return;
  inputs_.bounds = size;

  if (!layer_tree_host_)
    return;

  // Both bounds clipping and mask clipping can result in new areas of subtrees
  // being exposed on a bounds change. Ensure the damaged areas are updated.
  if (masks_to_bounds() || inputs_.mask_layer.get()) {
    SetSubtreePropertyChanged();
    SetPropertyTreesNeedRebuild();
  }

  if (scrollable()) {
    auto& scroll_tree = layer_tree_host_->property_trees()->scroll_tree;
    if (auto* scroll_node = scroll_tree.Node(scroll_tree_index_))
      scroll_node->bounds = inputs_.bounds;
    else
      SetPropertyTreesNeedRebuild();
  }

  SetNeedsCommit();
}

void Layer::SetOverscrollBehavior(const OverscrollBehavior& behavior) {
  DCHECK(IsPropertyChangeAllowed());
  if (overscroll_behavior() == behavior)
    return;
  inputs_.overscroll_behavior = behavior;
  if (!layer_tree_host_)
    return;

  if (scrollable()) {
    auto& scroll_tree = layer_tree_host_->property_trees()->scroll_tree;
    if (auto* scroll_node = scroll_tree.Node(scroll_tree_index_))
      scroll_node->overscroll_behavior = behavior;
    else
      SetPropertyTreesNeedRebuild();
  }

  SetNeedsCommit();
}

void Layer::SetSnapContainerData(base::Optional<SnapContainerData> data) {
  DCHECK(IsPropertyChangeAllowed());
  if (snap_container_data() == data)
    return;
  inputs_.snap_container_data = std::move(data);
  if (!layer_tree_host_)
    return;

  if (scrollable()) {
    auto& scroll_tree = layer_tree_host_->property_trees()->scroll_tree;
    if (auto* scroll_node = scroll_tree.Node(scroll_tree_index_))
      scroll_node->snap_container_data = inputs_.snap_container_data;
    else
      SetPropertyTreesNeedRebuild();
  }

  SetNeedsCommit();
}

Layer* Layer::RootLayer() {
  Layer* layer = this;
  while (layer->parent())
    layer = layer->parent();
  return layer;
}

void Layer::RemoveAllChildren() {
  DCHECK(IsPropertyChangeAllowed());
  while (inputs_.children.size()) {
    Layer* layer = inputs_.children[0].get();
    DCHECK_EQ(this, layer->parent());
    layer->RemoveFromParent();
  }
}

void Layer::SetChildren(const LayerList& children) {
  DCHECK(IsPropertyChangeAllowed());
  if (children == inputs_.children)
    return;

  RemoveAllChildren();
  for (size_t i = 0; i < children.size(); ++i)
    AddChild(children[i]);
}

bool Layer::HasAncestor(const Layer* ancestor) const {
  for (const Layer* layer = parent(); layer; layer = layer->parent()) {
    if (layer == ancestor)
      return true;
  }
  return false;
}

void Layer::RequestCopyOfOutput(
    std::unique_ptr<viz::CopyOutputRequest> request) {
  DCHECK(IsPropertyChangeAllowed());
  if (request->has_source()) {
    const base::UnguessableToken& source = request->source();
    auto it = std::find_if(
        inputs_.copy_requests.begin(), inputs_.copy_requests.end(),
        [&source](const std::unique_ptr<viz::CopyOutputRequest>& x) {
          return x->has_source() && x->source() == source;
        });
    if (it != inputs_.copy_requests.end())
      inputs_.copy_requests.erase(it);
  }
  inputs_.copy_requests.push_back(std::move(request));
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
  if (layer_tree_host_)
    layer_tree_host_->SetHasCopyRequest(true);
}

void Layer::SetSubtreeHasCopyRequest(bool subtree_has_copy_request) {
  subtree_has_copy_request_ = subtree_has_copy_request;
}

bool Layer::SubtreeHasCopyRequest() const {
  DCHECK(layer_tree_host_);
  // When the copy request is pushed to effect tree, we reset layer tree host's
  // has_copy_request but do not clear subtree_has_copy_request on individual
  // layers.
  return layer_tree_host_->has_copy_request() && subtree_has_copy_request_;
}

void Layer::SetBackgroundColor(SkColor background_color) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.background_color == background_color)
    return;
  inputs_.background_color = background_color;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetSafeOpaqueBackgroundColor(SkColor background_color) {
  DCHECK(IsPropertyChangeAllowed());
  if (safe_opaque_background_color_ == background_color)
    return;
  safe_opaque_background_color_ = background_color;
  SetNeedsPushProperties();
}

SkColor Layer::SafeOpaqueBackgroundColor() const {
  if (contents_opaque())
    return safe_opaque_background_color_;
  SkColor color = background_color();
  if (SkColorGetA(color) == 255)
    color = SK_ColorTRANSPARENT;
  return color;
}

void Layer::SetMasksToBounds(bool masks_to_bounds) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.masks_to_bounds == masks_to_bounds)
    return;
  inputs_.masks_to_bounds = masks_to_bounds;
  SetNeedsCommit();
  SetPropertyTreesNeedRebuild();
  SetSubtreePropertyChanged();
}

void Layer::SetMaskLayer(Layer* mask_layer) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.mask_layer.get() == mask_layer)
    return;
  if (inputs_.mask_layer.get()) {
    DCHECK_EQ(this, inputs_.mask_layer->parent());
    inputs_.mask_layer->RemoveFromParent();
  }
  inputs_.mask_layer = mask_layer;
  if (inputs_.mask_layer.get()) {
    // The mask layer should not have any children.
    DCHECK(inputs_.mask_layer->children().empty());

    inputs_.mask_layer->RemoveFromParent();
    DCHECK(!inputs_.mask_layer->parent());
    inputs_.mask_layer->SetParent(this);
    if (inputs_.filters.IsEmpty() && inputs_.background_filters.IsEmpty() &&
        (!layer_tree_host_ ||
         layer_tree_host_->GetSettings().enable_mask_tiling)) {
      inputs_.mask_layer->SetLayerMaskType(
          Layer::LayerMaskType::MULTI_TEXTURE_MASK);
    } else {
      inputs_.mask_layer->SetLayerMaskType(
          Layer::LayerMaskType::SINGLE_TEXTURE_MASK);
    }
  }
  SetSubtreePropertyChanged();
  SetNeedsFullTreeSync();
}

void Layer::SetFilters(const FilterOperations& filters) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.filters == filters)
    return;
  inputs_.filters = filters;
  if (inputs_.mask_layer && !filters.IsEmpty()) {
    inputs_.mask_layer->SetLayerMaskType(
        Layer::LayerMaskType::SINGLE_TEXTURE_MASK);
  }
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetBackgroundFilters(const FilterOperations& filters) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.background_filters == filters)
    return;
  inputs_.background_filters = filters;

  // We will not set the mask type to MULTI_TEXTURE_MASK if the mask layer's
  // filters are removed, because we do not want to reraster if the filters are
  // being animated.
  if (inputs_.mask_layer && !filters.IsEmpty()) {
    inputs_.mask_layer->SetLayerMaskType(
        Layer::LayerMaskType::SINGLE_TEXTURE_MASK);
  }
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetFiltersOrigin(const gfx::PointF& filters_origin) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.filters_origin == filters_origin)
    return;
  inputs_.filters_origin = filters_origin;
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetOpacity(float opacity) {
  DCHECK(IsPropertyChangeAllowed());
  DCHECK_GE(opacity, 0.f);
  DCHECK_LE(opacity, 1.f);

  if (inputs_.opacity == opacity)
    return;
  // We need to force a property tree rebuild when opacity changes from 1 to a
  // non-1 value or vice-versa as render surfaces can change.
  bool force_rebuild = opacity == 1.f || inputs_.opacity == 1.f;
  inputs_.opacity = opacity;
  SetSubtreePropertyChanged();
  if (layer_tree_host_ && !force_rebuild) {
    PropertyTrees* property_trees = layer_tree_host_->property_trees();
    if (EffectNode* node =
            property_trees->effect_tree.Node(effect_tree_index())) {
      node->opacity = opacity;
      node->effect_changed = true;
      property_trees->effect_tree.set_needs_update(true);
    }
  }
  if (force_rebuild)
    SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

float Layer::EffectiveOpacity() const {
  return inputs_.hide_layer_and_subtree ? 0.f : inputs_.opacity;
}

bool Layer::OpacityCanAnimateOnImplThread() const {
  return false;
}

void Layer::SetBlendMode(SkBlendMode blend_mode) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.blend_mode == blend_mode)
    return;

  // Allowing only blend modes that are defined in the CSS Compositing standard,
  // plus destination-in which is used to implement masks.
  // http://dev.w3.org/fxtf/compositing-1/#blending
  switch (blend_mode) {
    case SkBlendMode::kSrcOver:
    case SkBlendMode::kDstIn:
    case SkBlendMode::kScreen:
    case SkBlendMode::kOverlay:
    case SkBlendMode::kDarken:
    case SkBlendMode::kLighten:
    case SkBlendMode::kColorDodge:
    case SkBlendMode::kColorBurn:
    case SkBlendMode::kHardLight:
    case SkBlendMode::kSoftLight:
    case SkBlendMode::kDifference:
    case SkBlendMode::kExclusion:
    case SkBlendMode::kMultiply:
    case SkBlendMode::kHue:
    case SkBlendMode::kSaturation:
    case SkBlendMode::kColor:
    case SkBlendMode::kLuminosity:
      // supported blend modes
      break;
    case SkBlendMode::kClear:
    case SkBlendMode::kSrc:
    case SkBlendMode::kDst:
    case SkBlendMode::kDstOver:
    case SkBlendMode::kSrcIn:
    case SkBlendMode::kSrcOut:
    case SkBlendMode::kDstOut:
    case SkBlendMode::kSrcATop:
    case SkBlendMode::kDstATop:
    case SkBlendMode::kXor:
    case SkBlendMode::kPlus:
    case SkBlendMode::kModulate:
      // Porter Duff Compositing Operators are not yet supported
      // http://dev.w3.org/fxtf/compositing-1/#porterduffcompositingoperators
      NOTREACHED();
      return;
  }

  inputs_.blend_mode = blend_mode;
  SetNeedsCommit();
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
}

void Layer::SetIsRootForIsolatedGroup(bool root) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.is_root_for_isolated_group == root)
    return;
  inputs_.is_root_for_isolated_group = root;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetHitTestableWithoutDrawsContent(bool should_hit_test) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.hit_testable_without_draws_content == should_hit_test)
    return;
  inputs_.hit_testable_without_draws_content = should_hit_test;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetContentsOpaque(bool opaque) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.contents_opaque == opaque)
    return;
  inputs_.contents_opaque = opaque;
  SetNeedsCommit();
  SetSubtreePropertyChanged();
  SetPropertyTreesNeedRebuild();
}

void Layer::SetPosition(const gfx::PointF& position) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.position == position)
    return;
  inputs_.position = position;

  if (!layer_tree_host_)
    return;

  SetSubtreePropertyChanged();
  TransformNode* transform_node = GetTransformNode();
  if (transform_node) {
    transform_node->update_post_local_transform(position, transform_origin());
    transform_node->needs_local_transform_update = true;
    transform_node->transform_changed = true;
    layer_tree_host_->property_trees()->transform_tree.set_needs_update(true);
  } else {
    SetPropertyTreesNeedRebuild();
  }

  SetNeedsCommit();
}

bool Layer::IsContainerForFixedPositionLayers() const {
  return inputs_.is_container_for_fixed_position_layers;
}

bool Are2dAxisAligned(const gfx::Transform& a, const gfx::Transform& b) {
  if (a.IsScaleOrTranslation() && b.IsScaleOrTranslation()) {
    return true;
  }

  gfx::Transform inverse(gfx::Transform::kSkipInitialization);
  if (b.GetInverse(&inverse)) {
    inverse *= a;
    return inverse.Preserves2dAxisAlignment();
  } else {
    // TODO(weiliangc): Should return false because b is not invertible.
    return a.Preserves2dAxisAlignment();
  }
}

void Layer::SetTransform(const gfx::Transform& transform) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.transform == transform)
    return;

  SetSubtreePropertyChanged();
  if (layer_tree_host_) {
    if (has_transform_node_) {
      PropertyTrees* property_trees = layer_tree_host_->property_trees();
      if (TransformNode* transform_node =
              property_trees->transform_tree.Node(transform_tree_index())) {
        // We need to trigger a rebuild if we could have affected 2d axis
        // alignment. We'll check to see if transform and inputs_.transform are
        // axis align with respect to one another.
        DCHECK_EQ(transform_tree_index(), transform_node->id);
        bool preserves_2d_axis_alignment =
            Are2dAxisAligned(inputs_.transform, transform);
        transform_node->local = transform;
        transform_node->needs_local_transform_update = true;
        transform_node->transform_changed = true;
        layer_tree_host_->property_trees()->transform_tree.set_needs_update(
            true);
        if (!preserves_2d_axis_alignment)
          SetPropertyTreesNeedRebuild();
      }
    } else {
      SetPropertyTreesNeedRebuild();
    }
  }

  inputs_.transform = transform;
  SetNeedsCommit();
}

void Layer::SetTransformOrigin(const gfx::Point3F& transform_origin) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.transform_origin == transform_origin)
    return;
  inputs_.transform_origin = transform_origin;

  if (!layer_tree_host_)
    return;

  SetSubtreePropertyChanged();
  TransformNode* transform_node = GetTransformNode();
  if (transform_node) {
    DCHECK_EQ(transform_tree_index(), transform_node->id);
    transform_node->update_pre_local_transform(transform_origin);
    transform_node->update_post_local_transform(position(), transform_origin);
    transform_node->needs_local_transform_update = true;
    transform_node->transform_changed = true;
    layer_tree_host_->property_trees()->transform_tree.set_needs_update(true);
  } else {
    SetPropertyTreesNeedRebuild();
  }

  SetNeedsCommit();
}

bool Layer::ScrollOffsetAnimationWasInterrupted() const {
  return GetMutatorHost()->ScrollOffsetAnimationWasInterrupted(element_id());
}

void Layer::SetScrollParent(Layer* parent) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.scroll_parent == parent)
    return;

  inputs_.scroll_parent = parent;

  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetClipParent(Layer* ancestor) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.clip_parent == ancestor)
    return;

  if (inputs_.clip_parent)
    inputs_.clip_parent->RemoveClipChild(this);

  inputs_.clip_parent = ancestor;

  if (inputs_.clip_parent)
    inputs_.clip_parent->AddClipChild(this);

  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::AddClipChild(Layer* child) {
  if (!clip_children_)
    clip_children_.reset(new std::set<Layer*>);
  clip_children_->insert(child);
  SetNeedsCommit();
}

void Layer::RemoveClipChild(Layer* child) {
  clip_children_->erase(child);
  if (clip_children_->empty())
    clip_children_ = nullptr;
  SetNeedsCommit();
}

void Layer::SetScrollOffset(const gfx::ScrollOffset& scroll_offset) {
  DCHECK(IsPropertyChangeAllowed());

  if (inputs_.scroll_offset == scroll_offset)
    return;
  inputs_.scroll_offset = scroll_offset;

  if (!layer_tree_host_)
    return;

  UpdateScrollOffset(scroll_offset);

  SetNeedsCommit();
}

void Layer::SetScrollOffsetFromImplSide(
    const gfx::ScrollOffset& scroll_offset) {
  DCHECK(IsPropertyChangeAllowed());
  // This function only gets called during a BeginMainFrame, so there
  // is no need to call SetNeedsUpdate here.
  DCHECK(layer_tree_host_ && layer_tree_host_->CommitRequested());
  if (inputs_.scroll_offset == scroll_offset)
    return;
  inputs_.scroll_offset = scroll_offset;
  SetNeedsPushProperties();

  UpdateScrollOffset(scroll_offset);

  if (!inputs_.did_scroll_callback.is_null())
    inputs_.did_scroll_callback.Run(scroll_offset, element_id());

  // The callback could potentially change the layer structure:
  // "this" may have been destroyed during the process.
}

void Layer::UpdateScrollOffset(const gfx::ScrollOffset& scroll_offset) {
  DCHECK(scrollable());
  if (scroll_tree_index() == ScrollTree::kInvalidNodeId) {
    // Ensure the property trees just have not been built yet but are marked for
    // being built which will set the correct scroll offset values.
    DCHECK(layer_tree_host_->property_trees()->needs_rebuild);
    return;
  }

  // If a scroll node exists, it should have an associated transform node.
  DCHECK(transform_tree_index() != TransformTree::kInvalidNodeId);

  auto& property_trees = *layer_tree_host_->property_trees();
  property_trees.scroll_tree.SetScrollOffset(element_id(), scroll_offset);
  auto* transform_node =
      property_trees.transform_tree.Node(transform_tree_index());
  DCHECK_EQ(transform_tree_index(), transform_node->id);
  transform_node->scroll_offset = CurrentScrollOffset();
  transform_node->needs_local_transform_update = true;
  property_trees.transform_tree.set_needs_update(true);
}

void Layer::SetScrollable(const gfx::Size& bounds) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.scrollable && inputs_.scroll_container_bounds == bounds)
    return;
  bool was_scrollable = inputs_.scrollable;
  inputs_.scrollable = true;
  inputs_.scroll_container_bounds = bounds;

  if (!layer_tree_host_)
    return;

  auto& scroll_tree = layer_tree_host_->property_trees()->scroll_tree;
  auto* scroll_node = scroll_tree.Node(scroll_tree_index_);
  if (was_scrollable && scroll_node)
    scroll_node->container_bounds = inputs_.scroll_container_bounds;
  else
    SetPropertyTreesNeedRebuild();

  SetNeedsCommit();
}

void Layer::SetUserScrollable(bool horizontal, bool vertical) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.user_scrollable_horizontal == horizontal &&
      inputs_.user_scrollable_vertical == vertical)
    return;
  inputs_.user_scrollable_horizontal = horizontal;
  inputs_.user_scrollable_vertical = vertical;
  if (!layer_tree_host_)
    return;

  if (scrollable()) {
    auto& scroll_tree = layer_tree_host_->property_trees()->scroll_tree;
    if (auto* scroll_node = scroll_tree.Node(scroll_tree_index_)) {
      scroll_node->user_scrollable_horizontal = horizontal;
      scroll_node->user_scrollable_vertical = vertical;
    } else {
      SetPropertyTreesNeedRebuild();
    }
  }

  SetNeedsCommit();
}

void Layer::AddMainThreadScrollingReasons(
    uint32_t main_thread_scrolling_reasons) {
  DCHECK(IsPropertyChangeAllowed());
  DCHECK(main_thread_scrolling_reasons);
  // Layer should only see non-transient scrolling reasons. Transient scrolling
  // reasons are computed per hit test.
  DCHECK(MainThreadScrollingReason::MainThreadCanSetScrollReasons(
      main_thread_scrolling_reasons));
  uint32_t new_reasons =
      inputs_.main_thread_scrolling_reasons | main_thread_scrolling_reasons;
  if (inputs_.main_thread_scrolling_reasons == new_reasons)
    return;
  inputs_.main_thread_scrolling_reasons = new_reasons;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::ClearMainThreadScrollingReasons(
    uint32_t main_thread_scrolling_reasons_to_clear) {
  DCHECK(IsPropertyChangeAllowed());
  DCHECK(main_thread_scrolling_reasons_to_clear);
  uint32_t new_reasons = ~main_thread_scrolling_reasons_to_clear &
                         inputs_.main_thread_scrolling_reasons;
  if (new_reasons == inputs_.main_thread_scrolling_reasons)
    return;
  inputs_.main_thread_scrolling_reasons = new_reasons;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetNonFastScrollableRegion(const Region& region) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.non_fast_scrollable_region == region)
    return;
  inputs_.non_fast_scrollable_region = region;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetTouchActionRegion(TouchActionRegion touch_action_region) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.touch_action_region == touch_action_region)
    return;

  inputs_.touch_action_region = std::move(touch_action_region);
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetCacheRenderSurface(bool cache) {
  DCHECK(IsPropertyChangeAllowed());
  if (cache_render_surface_ == cache)
    return;
  cache_render_surface_ = cache;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetForceRenderSurfaceForTesting(bool force) {
  DCHECK(IsPropertyChangeAllowed());
  if (force_render_surface_for_testing_ == force)
    return;
  force_render_surface_for_testing_ = force;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetDoubleSided(bool double_sided) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.double_sided == double_sided)
    return;
  inputs_.double_sided = double_sided;
  SetNeedsCommit();
  SetPropertyTreesNeedRebuild();
  SetSubtreePropertyChanged();
}

void Layer::Set3dSortingContextId(int id) {
  DCHECK(IsPropertyChangeAllowed());
  if (id == inputs_.sorting_context_id)
    return;
  inputs_.sorting_context_id = id;
  SetNeedsCommit();
  SetPropertyTreesNeedRebuild();
  SetSubtreePropertyChanged();
}

void Layer::SetTransformTreeIndex(int index) {
  DCHECK(IsPropertyChangeAllowed());
  if (transform_tree_index_ == index)
    return;
  if (index == TransformTree::kInvalidNodeId)
    has_transform_node_ = false;
  transform_tree_index_ = index;
  SetNeedsPushProperties();
}

int Layer::transform_tree_index() const {
  if (!layer_tree_host_ ||
      layer_tree_host_->property_trees()->sequence_number !=
          property_tree_sequence_number_) {
    return TransformTree::kInvalidNodeId;
  }
  return transform_tree_index_;
}

void Layer::SetClipTreeIndex(int index) {
  DCHECK(IsPropertyChangeAllowed());
  if (clip_tree_index_ == index)
    return;
  clip_tree_index_ = index;
  SetNeedsPushProperties();
}

int Layer::clip_tree_index() const {
  if (!layer_tree_host_ ||
      layer_tree_host_->property_trees()->sequence_number !=
          property_tree_sequence_number_) {
    return ClipTree::kInvalidNodeId;
  }
  return clip_tree_index_;
}

void Layer::SetEffectTreeIndex(int index) {
  DCHECK(IsPropertyChangeAllowed());
  if (effect_tree_index_ == index)
    return;
  effect_tree_index_ = index;
  SetNeedsPushProperties();
}

int Layer::effect_tree_index() const {
  if (!layer_tree_host_ ||
      layer_tree_host_->property_trees()->sequence_number !=
          property_tree_sequence_number_) {
    return EffectTree::kInvalidNodeId;
  }
  return effect_tree_index_;
}

void Layer::SetScrollTreeIndex(int index) {
  DCHECK(IsPropertyChangeAllowed());
  if (scroll_tree_index_ == index)
    return;
  scroll_tree_index_ = index;
  SetNeedsPushProperties();
}

int Layer::scroll_tree_index() const {
  if (!layer_tree_host_ ||
      layer_tree_host_->property_trees()->sequence_number !=
          property_tree_sequence_number_) {
    return ScrollTree::kInvalidNodeId;
  }
  return scroll_tree_index_;
}

void Layer::InvalidatePropertyTreesIndices() {
  SetTransformTreeIndex(TransformTree::kInvalidNodeId);
  SetClipTreeIndex(ClipTree::kInvalidNodeId);
  SetEffectTreeIndex(EffectTree::kInvalidNodeId);
  SetScrollTreeIndex(ScrollTree::kInvalidNodeId);
}

void Layer::SetPropertyTreesNeedRebuild() {
  if (layer_tree_host_)
    layer_tree_host_->property_trees()->needs_rebuild = true;
}

void Layer::SetShouldFlattenTransform(bool should_flatten) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.should_flatten_transform == should_flatten)
    return;
  inputs_.should_flatten_transform = should_flatten;
  SetNeedsCommit();
  SetPropertyTreesNeedRebuild();
  SetSubtreePropertyChanged();
}

void Layer::SetUseParentBackfaceVisibility(bool use) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.use_parent_backface_visibility == use)
    return;
  inputs_.use_parent_backface_visibility = use;
  SetNeedsPushProperties();
}

void Layer::SetShouldCheckBackfaceVisibility(
    bool should_check_backface_visibility) {
  if (should_check_backface_visibility_ == should_check_backface_visibility)
    return;
  should_check_backface_visibility_ = should_check_backface_visibility;
  SetNeedsPushProperties();
}

void Layer::SetIsDrawable(bool is_drawable) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.is_drawable == is_drawable)
    return;

  inputs_.is_drawable = is_drawable;
  UpdateDrawsContent(HasDrawableContent());
}

void Layer::SetHideLayerAndSubtree(bool hide) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.hide_layer_and_subtree == hide)
    return;

  inputs_.hide_layer_and_subtree = hide;
  SetNeedsCommit();
  SetPropertyTreesNeedRebuild();
  SetSubtreePropertyChanged();
}

void Layer::SetNeedsDisplayRect(const gfx::Rect& dirty_rect) {
  if (dirty_rect.IsEmpty())
    return;

  SetNeedsPushProperties();
  inputs_.update_rect.Union(dirty_rect);

  if (DrawsContent() && layer_tree_host_ && !ignore_set_needs_commit_)
    layer_tree_host_->SetNeedsUpdateLayers();
}

bool Layer::DescendantIsFixedToContainerLayer() const {
  for (size_t i = 0; i < inputs_.children.size(); ++i) {
    if (inputs_.children[i]->inputs_.position_constraint.is_fixed_position() ||
        inputs_.children[i]->DescendantIsFixedToContainerLayer())
      return true;
  }
  return false;
}

void Layer::SetIsResizedByBrowserControls(bool resized) {
  if (inputs_.is_resized_by_browser_controls == resized)
    return;
  inputs_.is_resized_by_browser_controls = resized;

  SetNeedsCommit();
}

bool Layer::IsResizedByBrowserControls() const {
  return inputs_.is_resized_by_browser_controls;
}

void Layer::SetIsContainerForFixedPositionLayers(bool container) {
  if (inputs_.is_container_for_fixed_position_layers == container)
    return;
  inputs_.is_container_for_fixed_position_layers = container;

  if (layer_tree_host_ && layer_tree_host_->CommitRequested())
    return;

  // Only request a commit if we have a fixed positioned descendant.
  if (DescendantIsFixedToContainerLayer()) {
    SetPropertyTreesNeedRebuild();
    SetNeedsCommit();
  }
}

void Layer::SetPositionConstraint(const LayerPositionConstraint& constraint) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.position_constraint == constraint)
    return;
  inputs_.position_constraint = constraint;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetStickyPositionConstraint(
    const LayerStickyPositionConstraint& constraint) {
  DCHECK(IsPropertyChangeAllowed());
  if (inputs_.sticky_position_constraint == constraint)
    return;
  inputs_.sticky_position_constraint = constraint;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

void Layer::SetLayerClient(base::WeakPtr<LayerClient> client) {
  inputs_.client = std::move(client);
  inputs_.debug_info = nullptr;
}

bool Layer::IsSnapped() {
  return scrollable();
}

void Layer::PushPropertiesTo(LayerImpl* layer) {
  TRACE_EVENT0("cc", "Layer::PushPropertiesTo");
  DCHECK(layer_tree_host_);

  // The element id should be set first because other setters may
  // depend on it. Referencing element id on a layer is
  // deprecated. http://crbug.com/709137
  layer->SetElementId(inputs_.element_id);
  layer->SetHasTransformNode(has_transform_node_);
  layer->SetBackgroundColor(inputs_.background_color);
  layer->SetSafeOpaqueBackgroundColor(safe_opaque_background_color_);
  layer->SetBounds(inputs_.bounds);
  layer->SetDebugInfo(std::move(inputs_.debug_info));
  layer->SetTransformTreeIndex(transform_tree_index());
  layer->SetEffectTreeIndex(effect_tree_index());
  layer->SetClipTreeIndex(clip_tree_index());
  layer->SetScrollTreeIndex(scroll_tree_index());
  layer->set_offset_to_transform_parent(offset_to_transform_parent_);
  layer->SetDrawsContent(DrawsContent());
  layer->SetHitTestableWithoutDrawsContent(
      hit_testable_without_draws_content());
  // subtree_property_changed_ is propagated to all descendants while building
  // property trees. So, it is enough to check it only for the current layer.
  if (subtree_property_changed_)
    layer->NoteLayerPropertyChanged();
  layer->set_may_contain_video(may_contain_video_);
  layer->SetMasksToBounds(inputs_.masks_to_bounds);
  layer->set_main_thread_scrolling_reasons(
      inputs_.main_thread_scrolling_reasons);
  layer->SetNonFastScrollableRegion(inputs_.non_fast_scrollable_region);
  layer->SetTouchActionRegion(inputs_.touch_action_region);
  layer->SetContentsOpaque(inputs_.contents_opaque);
  layer->SetPosition(inputs_.position);
  layer->set_should_flatten_transform_from_property_tree(
      should_flatten_transform_from_property_tree_);
  layer->SetUseParentBackfaceVisibility(inputs_.use_parent_backface_visibility);
  layer->SetShouldCheckBackfaceVisibility(should_check_backface_visibility_);

  if (scrollable())
    layer->SetScrollable(inputs_.scroll_container_bounds);

  // The property trees must be safe to access because they will be used below
  // to call |SetScrollOffsetClobberActiveValue|.
  DCHECK(layer->layer_tree_impl()->lifecycle().AllowsPropertyTreeAccess());

  // When a scroll offset animation is interrupted the new scroll position on
  // the pending tree will clobber any impl-side scrolling occuring on the
  // active tree. To do so, avoid scrolling the pending tree along with it
  // instead of trying to undo that scrolling later.
  if (ScrollOffsetAnimationWasInterrupted())
    layer->layer_tree_impl()
        ->property_trees()
        ->scroll_tree.SetScrollOffsetClobberActiveValue(layer->element_id());

  if (needs_show_scrollbars_)
    layer->set_needs_show_scrollbars(true);

  // If the main thread commits multiple times before the impl thread actually
  // draws, then damage tracking will become incorrect if we simply clobber the
  // update_rect here. The LayerImpl's update_rect needs to accumulate (i.e.
  // union) any update changes that have occurred on the main thread.
  inputs_.update_rect.Union(layer->update_rect());
  layer->SetUpdateRect(inputs_.update_rect);

  layer->SetHasWillChangeTransformHint(has_will_change_transform_hint());
  layer->SetNeedsPushProperties();

  layer->SetTrilinearFiltering(trilinear_filtering());

  // Reset any state that should be cleared for the next update.
  needs_show_scrollbars_ = false;
  subtree_property_changed_ = false;
  inputs_.update_rect = gfx::Rect();

  if (mask_layer())
    DCHECK_EQ(bounds().ToString(), mask_layer()->bounds().ToString());
  layer_tree_host_->RemoveLayerShouldPushProperties(this);
}

void Layer::TakeCopyRequests(
    std::vector<std::unique_ptr<viz::CopyOutputRequest>>* requests) {
  for (std::unique_ptr<viz::CopyOutputRequest>& request :
       inputs_.copy_requests) {
    // Ensure the result callback is not invoked on the compositing thread.
    if (!request->has_result_task_runner()) {
      request->set_result_task_runner(
          layer_tree_host()->GetTaskRunnerProvider()->MainThreadTaskRunner());
    }
    if (request->has_area()) {
      request->set_area(
          gfx::IntersectRects(request->area(), gfx::Rect(bounds())));
    }
    requests->push_back(std::move(request));
  }

  inputs_.copy_requests.clear();
}

std::unique_ptr<LayerImpl> Layer::CreateLayerImpl(LayerTreeImpl* tree_impl) {
  return LayerImpl::Create(tree_impl, inputs_.layer_id);
}

bool Layer::DrawsContent() const {
  return draws_content_;
}

bool Layer::HasDrawableContent() const {
  return inputs_.is_drawable;
}

void Layer::UpdateDrawsContent(bool has_drawable_content) {
  bool draws_content = has_drawable_content;
  DCHECK(inputs_.is_drawable || !has_drawable_content);
  if (draws_content == draws_content_)
    return;

  if (parent())
    parent()->AddDrawableDescendants(draws_content ? 1 : -1);

  draws_content_ = draws_content;
  SetPropertyTreesNeedRebuild();
  SetNeedsCommit();
}

int Layer::NumDescendantsThatDrawContent() const {
  return num_descendants_that_draw_content_;
}

bool Layer::Update() {
  DCHECK(layer_tree_host_);
  return false;
}

bool Layer::HasSlowPaths() const {
  return false;
}

bool Layer::HasNonAAPaint() const {
  return false;
}

void Layer::UpdateDebugInfo() {
  DCHECK(frame_viewer_instrumentation::IsTracingLayerTreeSnapshots());
  if (inputs_.client)
    inputs_.debug_info = inputs_.client->TakeDebugInfo(this);
}

void Layer::SetSubtreePropertyChanged() {
  if (subtree_property_changed_)
    return;
  subtree_property_changed_ = true;
  SetNeedsPushProperties();
}

void Layer::SetMayContainVideo(bool yes) {
  if (may_contain_video_ == yes)
    return;
  may_contain_video_ = yes;
  SetNeedsPushProperties();
}

void Layer::SetScrollbarsHiddenFromImplSide(bool hidden) {
  if (inputs_.client)
    inputs_.client->DidChangeScrollbarsHiddenIfOverlay(hidden);
}

// On<Property>Animated is called due to an ongoing accelerated animation.
// Since this animation is also being run on the compositor thread, there
// is no need to request a commit to push this value over, so the value is
// set directly rather than by calling Set<Property>.
void Layer::OnFilterAnimated(const FilterOperations& filters) {
  inputs_.filters = filters;
}

void Layer::OnOpacityAnimated(float opacity) {
  inputs_.opacity = opacity;
}

TransformNode* Layer::GetTransformNode() const {
  return has_transform_node_
             ? layer_tree_host_->property_trees()->transform_tree.Node(
                   transform_tree_index_)
             : nullptr;
}

void Layer::OnTransformAnimated(const gfx::Transform& transform) {
  inputs_.transform = transform;
}

bool Layer::HasTickingAnimationForTesting() const {
  return layer_tree_host_
             ? GetMutatorHost()->HasTickingKeyframeModelForTesting(element_id())
             : false;
}

void Layer::SetHasWillChangeTransformHint(bool has_will_change) {
  if (inputs_.has_will_change_transform_hint == has_will_change)
    return;
  inputs_.has_will_change_transform_hint = has_will_change;
  SetNeedsCommit();
}

void Layer::SetTrilinearFiltering(bool trilinear_filtering) {
  if (inputs_.trilinear_filtering == trilinear_filtering)
    return;
  inputs_.trilinear_filtering = trilinear_filtering;
  SetNeedsCommit();
}

MutatorHost* Layer::GetMutatorHost() const {
  return layer_tree_host_ ? layer_tree_host_->mutator_host() : nullptr;
}

ElementListType Layer::GetElementTypeForAnimation() const {
  return ElementListType::ACTIVE;
}

ScrollbarLayerInterface* Layer::ToScrollbarLayer() {
  return nullptr;
}

void Layer::RemoveFromClipTree() {
  if (clip_children_.get()) {
    std::set<Layer*> copy = *clip_children_;
    for (std::set<Layer*>::iterator it = copy.begin(); it != copy.end(); ++it)
      (*it)->SetClipParent(nullptr);
  }

  DCHECK(!clip_children_);
  SetClipParent(nullptr);
}

void Layer::AddDrawableDescendants(int num) {
  DCHECK_GE(num_descendants_that_draw_content_, 0);
  DCHECK_GE(num_descendants_that_draw_content_ + num, 0);
  if (num == 0)
    return;
  num_descendants_that_draw_content_ += num;
  SetNeedsCommit();
  if (parent())
    parent()->AddDrawableDescendants(num);
}

void Layer::RunMicroBenchmark(MicroBenchmark* benchmark) {
  benchmark->RunOnLayer(this);
}

void Layer::SetElementId(ElementId id) {
  DCHECK(IsPropertyChangeAllowed());
  if ((layer_tree_host_ && layer_tree_host_->IsUsingLayerLists()) ||
      inputs_.element_id == id)
    return;
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"), "Layer::SetElementId",
               "element", id.AsValue().release());
  if (inputs_.element_id && layer_tree_host()) {
    layer_tree_host_->UnregisterElement(inputs_.element_id,
                                        ElementListType::ACTIVE);
  }

  inputs_.element_id = id;

  if (inputs_.element_id && layer_tree_host()) {
    layer_tree_host_->RegisterElement(inputs_.element_id,
                                      ElementListType::ACTIVE, this);
  }

  SetNeedsCommit();
}

bool Layer::has_copy_requests_in_target_subtree() {
  return layer_tree_host_->property_trees()
      ->effect_tree.Node(effect_tree_index())
      ->subtree_has_copy_request;
}

gfx::Transform Layer::ScreenSpaceTransform() const {
  DCHECK_NE(transform_tree_index_, TransformTree::kInvalidNodeId);
  return draw_property_utils::ScreenSpaceTransform(
      this, layer_tree_host_->property_trees()->transform_tree);
}

}  // namespace cc
