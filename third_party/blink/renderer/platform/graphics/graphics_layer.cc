/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/layers/layer.h"
#include "cc/layers/picture_image_layer.h"
#include "cc/layers/picture_layer.h"
#include "cc/paint/display_item_list.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_float_rect.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/platform/bindings/runtime_call_stats.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "third_party/blink/renderer/platform/drag_image.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/geometry_as_json.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/geometry/region.h"
#include "third_party/blink/renderer/platform/graphics/bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/compositing/composited_layer_raster_invalidator.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_chunks_to_cc_layer.h"
#include "third_party/blink/renderer/platform/graphics/compositor_filter_operations.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/graphics/link_highlight.h"
#include "third_party/blink/renderer/platform/graphics/logging_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/graphics/paint/property_tree_state.h"
#include "third_party/blink/renderer/platform/graphics/paint/raster_invalidation_tracking.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/json/json_values.h"
#include "third_party/blink/renderer/platform/scroll/scroll_snap_data.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/skia/include/core/SkMatrix44.h"

namespace blink {

std::unique_ptr<GraphicsLayer> GraphicsLayer::Create(
    GraphicsLayerClient& client) {
  return base::WrapUnique(new GraphicsLayer(client));
}

GraphicsLayer::GraphicsLayer(GraphicsLayerClient& client)
    : client_(client),
      background_color_(Color::kTransparent),
      opacity_(1),
      blend_mode_(BlendMode::kNormal),
      has_transform_origin_(false),
      contents_opaque_(false),
      prevent_contents_opaque_changes_(false),
      should_flatten_transform_(true),
      backface_visibility_(true),
      draws_content_(false),
      contents_visible_(true),
      is_root_for_isolated_group_(false),
      hit_testable_without_draws_content_(false),
      needs_check_raster_invalidation_(false),
      has_scroll_parent_(false),
      has_clip_parent_(false),
      painted_(false),
      painting_phase_(kGraphicsLayerPaintAllWithOverflowClip),
      parent_(nullptr),
      mask_layer_(nullptr),
      contents_clipping_mask_layer_(nullptr),
      paint_count_(0),
      contents_layer_(nullptr),
      contents_layer_id_(0),
      scrollable_area_(nullptr),
      rendering_context3d_(0),
      weak_ptr_factory_(this) {
#if DCHECK_IS_ON()
  client.VerifyNotPainting();
#endif
  layer_ = cc::PictureLayer::Create(this);
  layer_->SetIsDrawable(draws_content_ && contents_visible_);
  layer_->SetLayerClient(weak_ptr_factory_.GetWeakPtr());

  UpdateTrackingRasterInvalidations();
}

GraphicsLayer::~GraphicsLayer() {
  layer_->SetLayerClient(nullptr);
  SetContentsLayer(nullptr);
  for (size_t i = 0; i < link_highlights_.size(); ++i)
    link_highlights_[i]->ClearCurrentGraphicsLayer();
  link_highlights_.clear();

#if DCHECK_IS_ON()
  client_.VerifyNotPainting();
#endif

  RemoveAllChildren();
  RemoveFromParent();
  DCHECK(!parent_);
}

LayoutRect GraphicsLayer::VisualRect() const {
  LayoutRect bounds = LayoutRect(FloatPoint(), Size());
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(layer_state_);
    bounds.MoveBy(layer_state_->offset);
  } else {
    bounds.Move(OffsetFromLayoutObjectWithSubpixelAccumulation());
  }
  return bounds;
}

void GraphicsLayer::SetHasWillChangeTransformHint(
    bool has_will_change_transform) {
  layer_->SetHasWillChangeTransformHint(has_will_change_transform);
}

void GraphicsLayer::SetOverscrollBehavior(
    const cc::OverscrollBehavior& behavior) {
  layer_->SetOverscrollBehavior(behavior);
}

void GraphicsLayer::SetSnapContainerData(
    base::Optional<SnapContainerData> data) {
  layer_->SetSnapContainerData(std::move(data));
}

void GraphicsLayer::SetIsResizedByBrowserControls(
    bool is_resized_by_browser_controls) {
  CcLayer()->SetIsResizedByBrowserControls(is_resized_by_browser_controls);
}

void GraphicsLayer::SetIsContainerForFixedPositionLayers(bool is_container) {
  CcLayer()->SetIsContainerForFixedPositionLayers(is_container);
}

void GraphicsLayer::SetParent(GraphicsLayer* layer) {
#if DCHECK_IS_ON()
  DCHECK(!layer || !layer->HasAncestor(this));
#endif
  parent_ = layer;
}

#if DCHECK_IS_ON()

bool GraphicsLayer::HasAncestor(GraphicsLayer* ancestor) const {
  for (GraphicsLayer* curr = Parent(); curr; curr = curr->Parent()) {
    if (curr == ancestor)
      return true;
  }

  return false;
}

#endif

bool GraphicsLayer::SetChildren(const GraphicsLayerVector& new_children) {
  // If the contents of the arrays are the same, nothing to do.
  if (new_children == children_)
    return false;

  RemoveAllChildren();

  size_t list_size = new_children.size();
  for (size_t i = 0; i < list_size; ++i)
    AddChildInternal(new_children[i]);

  UpdateChildList();

  return true;
}

void GraphicsLayer::AddChildInternal(GraphicsLayer* child_layer) {
  DCHECK_NE(child_layer, this);

  if (child_layer->Parent())
    child_layer->RemoveFromParent();

  child_layer->SetParent(this);
  children_.push_back(child_layer);

  // Don't call updateChildList here, this function is used in cases where it
  // should not be called until all children are processed.
}

void GraphicsLayer::AddChild(GraphicsLayer* child_layer) {
  AddChildInternal(child_layer);
  UpdateChildList();
}

void GraphicsLayer::AddChildBelow(GraphicsLayer* child_layer,
                                  GraphicsLayer* sibling) {
  DCHECK_NE(child_layer, this);
  child_layer->RemoveFromParent();

  bool found = false;
  for (unsigned i = 0; i < children_.size(); i++) {
    if (sibling == children_[i]) {
      children_.insert(i, child_layer);
      found = true;
      break;
    }
  }

  child_layer->SetParent(this);

  if (!found)
    children_.push_back(child_layer);

  UpdateChildList();
}

void GraphicsLayer::RemoveAllChildren() {
  while (!children_.IsEmpty()) {
    GraphicsLayer* cur_layer = children_.back();
    DCHECK(cur_layer->Parent());
    cur_layer->RemoveFromParent();
  }
}

void GraphicsLayer::RemoveFromParent() {
  if (parent_) {
    // We use reverseFind so that removeAllChildren() isn't n^2.
    parent_->children_.EraseAt(parent_->children_.ReverseFind(this));
    SetParent(nullptr);
  }

  CcLayer()->RemoveFromParent();
}

void GraphicsLayer::SetOffsetFromLayoutObject(
    const IntSize& offset,
    ShouldSetNeedsDisplay should_set_needs_display) {
  SetOffsetDoubleFromLayoutObject(offset);
}

void GraphicsLayer::SetOffsetDoubleFromLayoutObject(
    const DoubleSize& offset,
    ShouldSetNeedsDisplay should_set_needs_display) {
  if (offset == offset_from_layout_object_)
    return;

  offset_from_layout_object_ = offset;
  CcLayer()->SetFiltersOrigin(FloatPoint() - ToFloatSize(offset));

  // If the compositing layer offset changes, we need to repaint.
  if (should_set_needs_display == kSetNeedsDisplay)
    SetNeedsDisplay();
}

LayoutSize GraphicsLayer::OffsetFromLayoutObjectWithSubpixelAccumulation()
    const {
  return LayoutSize(OffsetFromLayoutObject()) + client_.SubpixelAccumulation();
}

IntRect GraphicsLayer::InterestRect() {
  return previous_interest_rect_;
}

void GraphicsLayer::PaintRecursively() {
  Vector<GraphicsLayer*> repainted_layers;
  PaintRecursivelyInternal(repainted_layers);

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    // Notify the controllers that the artifact has been pushed and some
    // lifecycle state can be freed (such as raster invalidations).
    for (auto* layer : repainted_layers)
      layer->GetPaintController().FinishCycle();
  }

#if DCHECK_IS_ON()
  if (VLOG_IS_ON(2)) {
    static String s_previous_tree;
    LayerTreeFlags flags = VLOG_IS_ON(3) ? 0xffffffff : kOutputAsLayerTree;
    String new_tree = GetLayerTreeAsTextForTesting(flags);
    if (new_tree != s_previous_tree) {
      LOG(ERROR) << "After GraphicsLayer::PaintRecursively()\n"
                 << "GraphicsLayer tree:\n"
                 << new_tree.Utf8().data();
      s_previous_tree = new_tree;
    }
  }
#endif
}

void GraphicsLayer::PaintRecursivelyInternal(
    Vector<GraphicsLayer*>& repainted_layers) {
  if (DrawsContent()) {
    if (Paint(nullptr))
      repainted_layers.push_back(this);
  }

  if (MaskLayer())
    MaskLayer()->PaintRecursivelyInternal(repainted_layers);
  if (ContentsClippingMaskLayer())
    ContentsClippingMaskLayer()->PaintRecursivelyInternal(repainted_layers);

  for (auto* child : Children())
    child->PaintRecursivelyInternal(repainted_layers);
}

bool GraphicsLayer::Paint(const IntRect* interest_rect,
                          GraphicsContext::DisabledMode disabled_mode) {
#if !DCHECK_IS_ON()
  // TODO(crbug.com/853096): Investigate why we can ever reach here without
  // a valid layer state. Seems to only happen on Android builds.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() && !layer_state_)
    return false;
#endif

  if (PaintWithoutCommit(interest_rect, disabled_mode))
    GetPaintController().CommitNewDisplayItems();
  else if (!needs_check_raster_invalidation_)
    return false;

#if DCHECK_IS_ON()
  if (VLOG_IS_ON(2)) {
    LOG(ERROR) << "Painted GraphicsLayer: " << DebugName()
               << " interest_rect=" << InterestRect().ToString();
  }
#endif

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(layer_state_) << "No layer state for GraphicsLayer: " << DebugName();
    // Generate raster invalidations for SPv175 (but not SPv2).
    IntRect layer_bounds(layer_state_->offset, ExpandedIntSize(Size()));
    EnsureRasterInvalidator().Generate(GetPaintController().GetPaintArtifact(),
                                       layer_bounds, layer_state_->state,
                                       VisualRectSubpixelOffset());
  }

  if (RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
      DrawsContent()) {
    auto& tracking = EnsureRasterInvalidator().EnsureTracking();
    tracking.CheckUnderInvalidations(DebugName(), CapturePaintRecord(),
                                     InterestRect());
    if (auto record = tracking.UnderInvalidationRecord()) {
      // Add the under-invalidation overlay onto the painted result.
      GetPaintController().AppendDebugDrawingAfterCommit(
          *this, std::move(record),
          layer_state_ ? &layer_state_->state : nullptr);
      // Ensure the compositor will raster the under-invalidation overlay.
      layer_->SetNeedsDisplay();
    }
  }

  needs_check_raster_invalidation_ = false;
  return true;
}

bool GraphicsLayer::PaintWithoutCommit(
    const IntRect* interest_rect,
    GraphicsContext::DisabledMode disabled_mode) {
  DCHECK(DrawsContent());

  if (client_.ShouldThrottleRendering())
    return false;

  IncrementPaintCount();

  IntRect new_interest_rect;
  if (!interest_rect) {
    new_interest_rect =
        client_.ComputeInterestRect(this, previous_interest_rect_);
    interest_rect = &new_interest_rect;
  }

  if (!GetPaintController().SubsequenceCachingIsDisabled() &&
      !client_.NeedsRepaint(*this) &&
      !GetPaintController().CacheIsAllInvalid() &&
      previous_interest_rect_ == *interest_rect) {
    return false;
  }

  GraphicsContext context(GetPaintController(), disabled_mode, nullptr);
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(layer_state_) << "No layer state for GraphicsLayer: " << DebugName();
    GetPaintController().UpdateCurrentPaintChunkProperties(base::nullopt,
                                                           layer_state_->state);
  }

  previous_interest_rect_ = *interest_rect;
  client_.PaintContents(this, context, painting_phase_, *interest_rect);
  return true;
}

void GraphicsLayer::UpdateChildList() {
  cc::Layer* child_host = layer_.get();
  child_host->RemoveAllChildren();

  ClearContentsLayerIfUnregistered();

  if (contents_layer_) {
    // FIXME: Add the contents layer in the correct order with negative z-order
    // children. This does not currently cause visible rendering issues because
    // contents layers are only used for replaced elements that don't have
    // children.
    child_host->AddChild(contents_layer_);
  }

  for (size_t i = 0; i < children_.size(); ++i)
    child_host->AddChild(children_[i]->CcLayer());

  for (size_t i = 0; i < link_highlights_.size(); ++i)
    child_host->AddChild(link_highlights_[i]->Layer());
}

void GraphicsLayer::UpdateLayerIsDrawable() {
  // For the rest of the accelerated compositor code, there is no reason to make
  // a distinction between drawsContent and contentsVisible. So, for
  // m_layer->layer(), these two flags are combined here. |m_contentsLayer|
  // shouldn't receive the drawsContent flag, so it is only given
  // contentsVisible.

  layer_->SetIsDrawable(draws_content_ && contents_visible_);
  if (cc::Layer* contents_layer = ContentsLayerIfRegistered())
    contents_layer->SetIsDrawable(contents_visible_);

  if (draws_content_) {
    layer_->SetNeedsDisplay();
    for (size_t i = 0; i < link_highlights_.size(); ++i)
      link_highlights_[i]->Invalidate();
  }
}

void GraphicsLayer::UpdateContentsRect() {
  cc::Layer* contents_layer = ContentsLayerIfRegistered();
  if (!contents_layer)
    return;

  contents_layer->SetPosition(
      FloatPoint(contents_rect_.X(), contents_rect_.Y()));
  if (!image_layer_) {
    contents_layer->SetBounds(static_cast<gfx::Size>(contents_rect_.Size()));
  } else {
    DCHECK_EQ(image_layer_.get(), contents_layer_);
    // The image_layer_ has fixed bounds, and we apply bounds changes via the
    // transform instead. Since we never change the transform on the
    // |image_layer_| otherwise, we can assume it is identity and just apply
    // the bounds to it directly. Same thing for transform origin.
    DCHECK(image_layer_->transform_origin() == gfx::Point3F());

    if (contents_rect_.Size().IsEmpty() || image_size_.IsEmpty()) {
      image_layer_->SetTransform(gfx::Transform());
      contents_layer->SetBounds(static_cast<gfx::Size>(contents_rect_.Size()));
    } else {
      gfx::Transform image_transform;
      image_transform.Scale(
          static_cast<float>(contents_rect_.Width()) / image_size_.Width(),
          static_cast<float>(contents_rect_.Height()) / image_size_.Height());
      image_layer_->SetTransform(image_transform);
      image_layer_->SetBounds(static_cast<gfx::Size>(image_size_));
    }
  }

  if (contents_clipping_mask_layer_) {
    if (contents_clipping_mask_layer_->Size() != contents_rect_.Size()) {
      contents_clipping_mask_layer_->SetSize(contents_rect_.Size());
      contents_clipping_mask_layer_->SetNeedsDisplay();
    }
    contents_clipping_mask_layer_->SetPosition(FloatPoint());
    contents_clipping_mask_layer_->SetOffsetFromLayoutObject(
        OffsetFromLayoutObject() +
        IntSize(contents_rect_.Location().X(), contents_rect_.Location().Y()));
  }
}

static HashSet<int>* g_registered_layer_set;

void GraphicsLayer::RegisterContentsLayer(cc::Layer* layer) {
  if (!g_registered_layer_set)
    g_registered_layer_set = new HashSet<int>;
  CHECK(!g_registered_layer_set->Contains(layer->id()));
  g_registered_layer_set->insert(layer->id());
}

void GraphicsLayer::UnregisterContentsLayer(cc::Layer* layer) {
  DCHECK(g_registered_layer_set);
  CHECK(g_registered_layer_set->Contains(layer->id()));
  g_registered_layer_set->erase(layer->id());
  layer->SetLayerClient(nullptr);
}

void GraphicsLayer::SetContentsTo(cc::Layer* layer,
                                  bool prevent_contents_opaque_changes) {
  bool children_changed = false;
  if (layer) {
    DCHECK(g_registered_layer_set);
    CHECK(g_registered_layer_set->Contains(layer->id()));
    if (contents_layer_id_ != layer->id()) {
      SetupContentsLayer(layer);
      children_changed = true;
    }
    UpdateContentsRect();
    prevent_contents_opaque_changes_ = prevent_contents_opaque_changes;
  } else {
    if (contents_layer_) {
      children_changed = true;

      // The old contents layer will be removed via updateChildList.
      SetContentsLayer(nullptr);
    }
  }

  if (children_changed)
    UpdateChildList();
}

void GraphicsLayer::SetupContentsLayer(cc::Layer* contents_layer) {
  DCHECK(contents_layer);
  SetContentsLayer(contents_layer);

  contents_layer_->SetTransformOrigin(FloatPoint3D());
  contents_layer_->SetUseParentBackfaceVisibility(true);

  // It is necessary to call SetDrawsContent() as soon as we receive the new
  // contents_layer, for the correctness of early exit conditions in
  // SetDrawsContent() and SetContentsVisible().
  contents_layer_->SetIsDrawable(contents_visible_);

  // Insert the content layer first. Video elements require this, because they
  // have shadow content that must display in front of the video.
  layer_->InsertChild(contents_layer_, 0);
  cc::Layer* border_cc_layer = contents_clipping_mask_layer_
                                   ? contents_clipping_mask_layer_->CcLayer()
                                   : nullptr;
  contents_layer_->SetMaskLayer(border_cc_layer);

  contents_layer_->Set3dSortingContextId(rendering_context3d_);
}

void GraphicsLayer::ClearContentsLayerIfUnregistered() {
  if (!contents_layer_id_ ||
      g_registered_layer_set->Contains(contents_layer_id_))
    return;

  SetContentsLayer(nullptr);
}

void GraphicsLayer::SetContentsLayer(cc::Layer* contents_layer) {
  // If we have a previous contents layer which is still registered, then unset
  // this client pointer. If unregistered, it has already nulled out the client
  // pointer and may have been deleted.
  if (contents_layer_ && g_registered_layer_set->Contains(contents_layer_id_))
    contents_layer_->SetLayerClient(nullptr);
  contents_layer_ = contents_layer;
  if (!contents_layer_) {
    contents_layer_id_ = 0;
    return;
  }
  contents_layer_->SetLayerClient(weak_ptr_factory_.GetWeakPtr());
  contents_layer_id_ = contents_layer_->id();
}

cc::Layer* GraphicsLayer::ContentsLayerIfRegistered() {
  ClearContentsLayerIfUnregistered();
  return contents_layer_;
}

CompositedLayerRasterInvalidator& GraphicsLayer::EnsureRasterInvalidator() {
  if (!raster_invalidator_) {
    raster_invalidator_ = std::make_unique<CompositedLayerRasterInvalidator>(
        [this](const IntRect& r) { SetNeedsDisplayInRectInternal(r); });
    raster_invalidator_->SetTracksRasterInvalidations(
        client_.IsTrackingRasterInvalidations());
  }
  return *raster_invalidator_;
}

void GraphicsLayer::UpdateTrackingRasterInvalidations() {
  bool should_track = client_.IsTrackingRasterInvalidations();
  if (should_track)
    EnsureRasterInvalidator().SetTracksRasterInvalidations(true);
  else if (raster_invalidator_)
    raster_invalidator_->SetTracksRasterInvalidations(false);

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() && paint_controller_)
    paint_controller_->SetTracksRasterInvalidations(should_track);
}

void GraphicsLayer::ResetTrackedRasterInvalidations() {
  if (auto* tracking = GetRasterInvalidationTracking())
    tracking->ClearInvalidations();
}

bool GraphicsLayer::HasTrackedRasterInvalidations() const {
  if (auto* tracking = GetRasterInvalidationTracking())
    return tracking->HasInvalidations();
  return false;
}

RasterInvalidationTracking* GraphicsLayer::GetRasterInvalidationTracking()
    const {
  return raster_invalidator_ ? raster_invalidator_->GetTracking() : nullptr;
}

void GraphicsLayer::TrackRasterInvalidation(const DisplayItemClient& client,
                                            const IntRect& rect,
                                            PaintInvalidationReason reason) {
  if (RasterInvalidationTracking::ShouldAlwaysTrack())
    EnsureRasterInvalidator().EnsureTracking();

  // For SPv175, this only tracks invalidations that the cc::Layer is fully
  // invalidated directly, e.g. from SetContentsNeedsDisplay(), etc.
  if (auto* tracking = GetRasterInvalidationTracking())
    tracking->AddInvalidation(&client, client.DebugName(), rect, reason);
}

static String PointerAsString(const void* ptr) {
  WTF::TextStream ts;
  ts << ptr;
  return ts.Release();
}

class GraphicsLayer::LayersAsJSONArray {
 public:
  LayersAsJSONArray(LayerTreeFlags flags)
      : flags_(flags),
        next_transform_id_(1),
        layers_json_(JSONArray::Create()),
        transforms_json_(JSONArray::Create()) {}

  // Outputs the layer tree rooted at |layer| as a JSON array, in paint order,
  // and the transform tree also as a JSON array.
  std::unique_ptr<JSONObject> operator()(const GraphicsLayer& layer) {
    auto json = JSONObject::Create();
    Walk(layer, 0, FloatPoint());
    json->SetArray("layers", std::move(layers_json_));
    if (transforms_json_->size())
      json->SetArray("transforms", std::move(transforms_json_));
    return json;
  }

  JSONObject* AddTransformJSON(int& transform_id) {
    auto transform_json = JSONObject::Create();
    int parent_transform_id = transform_id;
    transform_id = next_transform_id_++;
    transform_json->SetInteger("id", transform_id);
    if (parent_transform_id)
      transform_json->SetInteger("parent", parent_transform_id);
    auto* result = transform_json.get();
    transforms_json_->PushObject(std::move(transform_json));
    return result;
  }

  static FloatPoint ScrollPosition(const GraphicsLayer& layer) {
    if (const auto* scrollable_area = layer.GetScrollableArea())
      return scrollable_area->ScrollPosition();
    return FloatPoint();
  }

  void AddLayer(const GraphicsLayer& layer,
                int& transform_id,
                FloatPoint& position) {
    FloatPoint scroll_position = ScrollPosition(layer);
    if (scroll_position != FloatPoint()) {
      // Output scroll position as a transform.
      auto* scroll_translate_json = AddTransformJSON(transform_id);
      scroll_translate_json->SetArray(
          "transform", TransformAsJSONArray(TransformationMatrix().Translate(
                           -scroll_position.X(), -scroll_position.Y())));
      layer.AddFlattenInheritedTransformJSON(*scroll_translate_json);
    }

    if (!layer.transform_.IsIdentity() || layer.rendering_context3d_ ||
        layer.GetCompositingReasons() & CompositingReason::k3DTransform) {
      if (position != FloatPoint()) {
        // Output position offset as a transform.
        auto* position_translate_json = AddTransformJSON(transform_id);
        position_translate_json->SetArray(
            "transform", TransformAsJSONArray(TransformationMatrix().Translate(
                             position.X(), position.Y())));
        layer.AddFlattenInheritedTransformJSON(*position_translate_json);
        if (layer.Parent() && !layer.Parent()->should_flatten_transform_) {
          position_translate_json->SetBoolean("flattenInheritedTransform",
                                              false);
        }
        position = FloatPoint();
      }

      if (!layer.transform_.IsIdentity() || layer.rendering_context3d_) {
        auto* transform_json = AddTransformJSON(transform_id);
        layer.AddTransformJSONProperties(*transform_json,
                                         rendering_context_map_);
      }
    }

    auto json =
        layer.LayerAsJSONInternal(flags_, rendering_context_map_, position);
    if (transform_id)
      json->SetInteger("transform", transform_id);
    layers_json_->PushObject(std::move(json));
  }

  void Walk(const GraphicsLayer& layer,
            int parent_transform_id,
            const FloatPoint& parent_position) {
    FloatPoint position = parent_position + layer.position_;
    int transform_id = parent_transform_id;
    AddLayer(layer, transform_id, position);
    for (auto* const child : layer.children_)
      Walk(*child, transform_id, position);
  }

 private:
  LayerTreeFlags flags_;
  int next_transform_id_;
  RenderingContextMap rendering_context_map_;
  std::unique_ptr<JSONArray> layers_json_;
  std::unique_ptr<JSONArray> transforms_json_;
};

std::unique_ptr<JSONObject> GraphicsLayer::LayerTreeAsJSON(
    LayerTreeFlags flags) const {
  if (flags & kOutputAsLayerTree) {
    RenderingContextMap rendering_context_map;
    return LayerTreeAsJSONInternal(flags, rendering_context_map);
  }

  return LayersAsJSONArray(flags)(*this);
}

// This is the SPv1 version of ContentLayerClientImpl::LayerAsJSON().
std::unique_ptr<JSONObject> GraphicsLayer::LayerAsJSONInternal(
    LayerTreeFlags flags,
    RenderingContextMap& rendering_context_map,
    const FloatPoint& position) const {
  std::unique_ptr<JSONObject> json = JSONObject::Create();

  if (flags & kLayerTreeIncludesDebugInfo)
    json->SetString("this", PointerAsString(this));

  json->SetString("name", DebugName());

  if (position != FloatPoint())
    json->SetArray("position", PointAsJSONArray(position));

  if (flags & kLayerTreeIncludesDebugInfo &&
      offset_from_layout_object_ != DoubleSize()) {
    json->SetArray("offsetFromLayoutObject",
                   SizeAsJSONArray(offset_from_layout_object_));
  }

  if (size_ != IntSize())
    json->SetArray("bounds", SizeAsJSONArray(size_));

  if (opacity_ != 1)
    json->SetDouble("opacity", opacity_);

  if (blend_mode_ != BlendMode::kNormal) {
    json->SetString("blendMode",
                    CompositeOperatorName(kCompositeSourceOver, blend_mode_));
  }

  if (is_root_for_isolated_group_)
    json->SetBoolean("isolate", true);

  if (contents_opaque_)
    json->SetBoolean("contentsOpaque", true);

  if (!draws_content_)
    json->SetBoolean("drawsContent", false);

  if (!contents_visible_)
    json->SetBoolean("contentsVisible", false);

  if (!backface_visibility_)
    json->SetString("backfaceVisibility", "hidden");

  if (flags & kLayerTreeIncludesDebugInfo)
    json->SetString("client", PointerAsString(&client_));

  if (background_color_.Alpha()) {
    json->SetString("backgroundColor",
                    background_color_.NameForLayoutTreeAsText());
  }

  if (flags & kOutputAsLayerTree) {
    AddTransformJSONProperties(*json, rendering_context_map);
    if (!should_flatten_transform_)
      json->SetBoolean("shouldFlattenTransform", false);
    if (scrollable_area_ &&
        scrollable_area_->ScrollPosition() != FloatPoint()) {
      json->SetArray("scrollPosition",
                     PointAsJSONArray(scrollable_area_->ScrollPosition()));
    }
  }

  if ((flags & kLayerTreeIncludesPaintInvalidations) &&
      client_.IsTrackingRasterInvalidations() &&
      GetRasterInvalidationTracking())
    GetRasterInvalidationTracking()->AsJSON(json.get());

  if ((flags & kLayerTreeIncludesPaintingPhases) && painting_phase_) {
    std::unique_ptr<JSONArray> painting_phases_json = JSONArray::Create();
    if (painting_phase_ & kGraphicsLayerPaintBackground)
      painting_phases_json->PushString("GraphicsLayerPaintBackground");
    if (painting_phase_ & kGraphicsLayerPaintForeground)
      painting_phases_json->PushString("GraphicsLayerPaintForeground");
    if (painting_phase_ & kGraphicsLayerPaintMask)
      painting_phases_json->PushString("GraphicsLayerPaintMask");
    if (painting_phase_ & kGraphicsLayerPaintChildClippingMask)
      painting_phases_json->PushString("GraphicsLayerPaintChildClippingMask");
    if (painting_phase_ & kGraphicsLayerPaintAncestorClippingMask)
      painting_phases_json->PushString(
          "GraphicsLayerPaintAncestorClippingMask");
    if (painting_phase_ & kGraphicsLayerPaintOverflowContents)
      painting_phases_json->PushString("GraphicsLayerPaintOverflowContents");
    if (painting_phase_ & kGraphicsLayerPaintCompositedScroll)
      painting_phases_json->PushString("GraphicsLayerPaintCompositedScroll");
    if (painting_phase_ & kGraphicsLayerPaintDecoration)
      painting_phases_json->PushString("GraphicsLayerPaintDecoration");
    json->SetArray("paintingPhases", std::move(painting_phases_json));
  }

  if (flags & kLayerTreeIncludesClipAndScrollParents) {
    if (has_scroll_parent_)
      json->SetBoolean("hasScrollParent", true);
    if (has_clip_parent_)
      json->SetBoolean("hasClipParent", true);
  }

  if (flags &
      (kLayerTreeIncludesDebugInfo | kLayerTreeIncludesCompositingReasons)) {
    bool debug = flags & kLayerTreeIncludesDebugInfo;
    {
      std::unique_ptr<JSONArray> compositing_reasons_json = JSONArray::Create();
      auto names = debug ? CompositingReason::Descriptions(compositing_reasons_)
                         : CompositingReason::ShortNames(compositing_reasons_);
      for (const char* name : names)
        compositing_reasons_json->PushString(name);
      json->SetArray("compositingReasons", std::move(compositing_reasons_json));
    }
    {
      std::unique_ptr<JSONArray> squashing_disallowed_reasons_json =
          JSONArray::Create();
      auto names = debug ? SquashingDisallowedReason::Descriptions(
                               squashing_disallowed_reasons_)
                         : SquashingDisallowedReason::ShortNames(
                               squashing_disallowed_reasons_);
      for (const char* name : names)
        squashing_disallowed_reasons_json->PushString(name);
      json->SetArray("squashingDisallowedReasons",
                     std::move(squashing_disallowed_reasons_json));
    }
  }

  if (mask_layer_) {
    std::unique_ptr<JSONArray> mask_layer_json = JSONArray::Create();
    mask_layer_json->PushObject(mask_layer_->LayerAsJSONInternal(
        flags, rendering_context_map, mask_layer_->position_));
    json->SetArray("maskLayer", std::move(mask_layer_json));
  }

  if (contents_clipping_mask_layer_) {
    std::unique_ptr<JSONArray> contents_clipping_mask_layer_json =
        JSONArray::Create();
    contents_clipping_mask_layer_json->PushObject(
        contents_clipping_mask_layer_->LayerAsJSONInternal(
            flags, rendering_context_map,
            contents_clipping_mask_layer_->position_));
    json->SetArray("contentsClippingMaskLayer",
                   std::move(contents_clipping_mask_layer_json));
  }

  if (layer_state_ && (flags & (kLayerTreeIncludesDebugInfo |
                                kLayerTreeIncludesPaintRecords))) {
    json->SetString("layerState", layer_state_->state.ToString());
    json->SetValue("layerOffset", PointAsJSONArray(layer_state_->offset));
  }

#if DCHECK_IS_ON()
  if (DrawsContent() && (flags & kLayerTreeIncludesPaintRecords))
    json->SetValue("paintRecord", RecordAsJSON(*CapturePaintRecord()));
#endif

  return json;
}

std::unique_ptr<JSONObject> GraphicsLayer::LayerTreeAsJSONInternal(
    LayerTreeFlags flags,
    RenderingContextMap& rendering_context_map) const {
  std::unique_ptr<JSONObject> json =
      LayerAsJSONInternal(flags, rendering_context_map, position_);

  if (children_.size()) {
    std::unique_ptr<JSONArray> children_json = JSONArray::Create();
    for (size_t i = 0; i < children_.size(); i++) {
      children_json->PushObject(
          children_[i]->LayerTreeAsJSONInternal(flags, rendering_context_map));
    }
    json->SetArray("children", std::move(children_json));
  }

  return json;
}

void GraphicsLayer::AddTransformJSONProperties(
    JSONObject& json,
    RenderingContextMap& rendering_context_map) const {
  if (!transform_.IsIdentity())
    json.SetArray("transform", TransformAsJSONArray(transform_));

  if (!transform_.IsIdentityOrTranslation())
    json.SetArray("origin", PointAsJSONArray(transform_origin_));

  AddFlattenInheritedTransformJSON(json);

  if (rendering_context3d_) {
    auto it = rendering_context_map.find(rendering_context3d_);
    int context_id = rendering_context_map.size() + 1;
    if (it == rendering_context_map.end())
      rendering_context_map.Set(rendering_context3d_, context_id);
    else
      context_id = it->value;

    json.SetInteger("renderingContext", context_id);
  }
}

void GraphicsLayer::AddFlattenInheritedTransformJSON(JSONObject& json) const {
  if (Parent() && !Parent()->should_flatten_transform_)
    json.SetBoolean("flattenInheritedTransform", false);
}

String GraphicsLayer::GetLayerTreeAsTextForTesting(LayerTreeFlags flags) const {
  return LayerTreeAsJSON(flags)->ToPrettyJSONString();
}

String GraphicsLayer::DebugName(cc::Layer* layer) const {
  if (layer->id() == contents_layer_id_)
    return "ContentsLayer for " + client_.DebugName(this);

  for (size_t i = 0; i < link_highlights_.size(); ++i) {
    if (layer == link_highlights_[i]->Layer()) {
      return "LinkHighlight[" + String::Number(i) + "] for " +
             client_.DebugName(this);
    }
  }

  if (layer == layer_.get())
    return client_.DebugName(this);

  NOTREACHED();
  return "";
}

void GraphicsLayer::SetPosition(const FloatPoint& point) {
  position_ = point;
  CcLayer()->SetPosition(position_);
}

void GraphicsLayer::SetSize(const IntSize& size) {
  // We are receiving negative sizes here that cause assertions to fail in the
  // compositor. Clamp them to 0 to avoid those assertions.
  // FIXME: This should be an DCHECK instead, as negative sizes should not exist
  // in WebCore.
  auto clamped_size = size;
  clamped_size.ClampNegativeToZero();

  if (clamped_size == size_)
    return;

  size_ = clamped_size;

  // Invalidate the layer as a DisplayItemClient.
  SetDisplayItemsUncached();

  layer_->SetBounds(static_cast<gfx::Size>(size_));
  // Note that we don't resize m_contentsLayer. It's up the caller to do that.
}

void GraphicsLayer::SetTransform(const TransformationMatrix& transform) {
  transform_ = transform;
  CcLayer()->SetTransform(TransformationMatrix::ToTransform(transform));
}

void GraphicsLayer::SetTransformOrigin(const FloatPoint3D& transform_origin) {
  has_transform_origin_ = true;
  transform_origin_ = transform_origin;
  CcLayer()->SetTransformOrigin(transform_origin);
}

void GraphicsLayer::SetShouldFlattenTransform(bool should_flatten) {
  if (should_flatten == should_flatten_transform_)
    return;

  should_flatten_transform_ = should_flatten;

  layer_->SetShouldFlattenTransform(should_flatten);
}

void GraphicsLayer::SetRenderingContext(int context) {
  if (rendering_context3d_ == context)
    return;

  rendering_context3d_ = context;
  layer_->Set3dSortingContextId(context);

  if (contents_layer_)
    contents_layer_->Set3dSortingContextId(rendering_context3d_);
}

bool GraphicsLayer::MasksToBounds() const {
  return layer_->masks_to_bounds();
}

void GraphicsLayer::SetMasksToBounds(bool masks_to_bounds) {
  layer_->SetMasksToBounds(masks_to_bounds);
}

void GraphicsLayer::SetDrawsContent(bool draws_content) {
  // NOTE: This early-exit is only correct because we also properly call
  // cc::Layer::SetIsDrawable() whenever |contents_layer_| is set to a new
  // layer in SetupContentsLayer().
  if (draws_content == draws_content_)
    return;

  draws_content_ = draws_content;
  UpdateLayerIsDrawable();

  if (!draws_content) {
    paint_controller_.reset();
    raster_invalidator_.reset();
  }
}

void GraphicsLayer::SetContentsVisible(bool contents_visible) {
  // NOTE: This early-exit is only correct because we also properly call
  // cc::Layer::SetIsDrawable() whenever |contents_layer_| is set to a new
  // layer in SetupContentsLayer().
  if (contents_visible == contents_visible_)
    return;

  contents_visible_ = contents_visible;
  UpdateLayerIsDrawable();
}

void GraphicsLayer::SetClipParent(cc::Layer* parent) {
  has_clip_parent_ = !!parent;
  layer_->SetClipParent(parent);
}

void GraphicsLayer::SetScrollParent(cc::Layer* parent) {
  has_scroll_parent_ = !!parent;
  layer_->SetScrollParent(parent);
}

void GraphicsLayer::SetBackgroundColor(const Color& color) {
  if (color == background_color_)
    return;

  background_color_ = color;
  layer_->SetBackgroundColor(background_color_.Rgb());
}

void GraphicsLayer::SetContentsOpaque(bool opaque) {
  contents_opaque_ = opaque;
  layer_->SetContentsOpaque(contents_opaque_);
  ClearContentsLayerIfUnregistered();
  if (contents_layer_ && !prevent_contents_opaque_changes_)
    contents_layer_->SetContentsOpaque(opaque);
}

void GraphicsLayer::SetMaskLayer(GraphicsLayer* mask_layer) {
  if (mask_layer == mask_layer_)
    return;

  mask_layer_ = mask_layer;
  layer_->SetMaskLayer(mask_layer_ ? mask_layer_->CcLayer() : nullptr);
}

void GraphicsLayer::SetContentsClippingMaskLayer(
    GraphicsLayer* contents_clipping_mask_layer) {
  if (contents_clipping_mask_layer == contents_clipping_mask_layer_)
    return;

  contents_clipping_mask_layer_ = contents_clipping_mask_layer;
  cc::Layer* contents_layer = ContentsLayerIfRegistered();
  if (!contents_layer)
    return;
  cc::Layer* contents_clipping_mask_cc_layer =
      contents_clipping_mask_layer_ ? contents_clipping_mask_layer_->CcLayer()
                                    : nullptr;
  contents_layer->SetMaskLayer(contents_clipping_mask_cc_layer);
  UpdateContentsRect();
}

void GraphicsLayer::SetBackfaceVisibility(bool visible) {
  backface_visibility_ = visible;
  CcLayer()->SetDoubleSided(backface_visibility_);
}

void GraphicsLayer::SetOpacity(float opacity) {
  float clamped_opacity = clampTo(opacity, 0.0f, 1.0f);
  opacity_ = clamped_opacity;
  CcLayer()->SetOpacity(opacity);
}

void GraphicsLayer::SetBlendMode(BlendMode blend_mode) {
  if (blend_mode_ == blend_mode)
    return;
  blend_mode_ = blend_mode;
  CcLayer()->SetBlendMode(WebCoreBlendModeToSkBlendMode(blend_mode));
}

void GraphicsLayer::SetIsRootForIsolatedGroup(bool isolated) {
  if (is_root_for_isolated_group_ == isolated)
    return;
  is_root_for_isolated_group_ = isolated;
  CcLayer()->SetIsRootForIsolatedGroup(isolated);
}

void GraphicsLayer::SetHitTestableWithoutDrawsContent(bool should_hit_test) {
  if (hit_testable_without_draws_content_ == should_hit_test)
    return;
  hit_testable_without_draws_content_ = should_hit_test;
  CcLayer()->SetHitTestableWithoutDrawsContent(should_hit_test);
}

void GraphicsLayer::SetContentsNeedsDisplay() {
  if (cc::Layer* contents_layer = ContentsLayerIfRegistered()) {
    contents_layer->SetNeedsDisplay();
    TrackRasterInvalidation(*this, contents_rect_,
                            PaintInvalidationReason::kFull);
  }
}

void GraphicsLayer::SetNeedsDisplay() {
  if (!DrawsContent())
    return;

  // TODO(chrishtr): Stop invalidating the rects once
  // FrameView::paintRecursively() does so.
  layer_->SetNeedsDisplay();
  for (size_t i = 0; i < link_highlights_.size(); ++i)
    link_highlights_[i]->Invalidate();
  GetPaintController().InvalidateAll();

  TrackRasterInvalidation(*this, IntRect(IntPoint(), size_),
                          PaintInvalidationReason::kFull);
}

DISABLE_CFI_PERF
void GraphicsLayer::SetNeedsDisplayInRect(
    const IntRect& rect,
    PaintInvalidationReason invalidation_reason,
    const DisplayItemClient& client) {
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  if (!DrawsContent())
    return;

  if (!ScopedSetNeedsDisplayInRectForTrackingOnly::s_enabled_)
    SetNeedsDisplayInRectInternal(rect);

  TrackRasterInvalidation(client, rect, invalidation_reason);
}

void GraphicsLayer::SetNeedsDisplayInRectInternal(const IntRect& rect) {
  DCHECK(DrawsContent());

  layer_->SetNeedsDisplayRect(rect);
  for (auto* link_highlight : link_highlights_)
    link_highlight->Invalidate();
}

void GraphicsLayer::SetContentsRect(const IntRect& rect) {
  if (rect == contents_rect_)
    return;

  contents_rect_ = rect;
  UpdateContentsRect();
}

void GraphicsLayer::SetContentsToImage(
    Image* image,
    Image::ImageDecodingMode decode_mode,
    RespectImageOrientationEnum respect_image_orientation) {
  PaintImage paint_image;
  if (image)
    paint_image = image->PaintImageForCurrentFrame();

  ImageOrientation image_orientation = kOriginTopLeft;
  SkMatrix matrix;
  if (paint_image && image->IsBitmapImage() &&
      respect_image_orientation == kRespectImageOrientation) {
    image_orientation = ToBitmapImage(image)->CurrentFrameOrientation();
    image_size_ = IntSize(paint_image.width(), paint_image.height());
    if (image_orientation.UsesWidthAsHeight())
      image_size_ = image_size_.TransposedSize();
    auto affine =
        image_orientation.TransformFromDefault(FloatSize(image_size_));
    auto transform = affine.ToTransformationMatrix();
    matrix = TransformationMatrix::ToSkMatrix44(transform);
  } else if (paint_image) {
    matrix = SkMatrix::I();
    image_size_ = IntSize(paint_image.width(), paint_image.height());
  } else {
    matrix = SkMatrix::I();
    image_size_ = IntSize();
  }

  if (paint_image) {
    paint_image =
        PaintImageBuilder::WithCopy(std::move(paint_image))
            .set_decoding_mode(Image::ToPaintImageDecodingMode(decode_mode))
            .TakePaintImage();
    if (!image_layer_) {
      image_layer_ = cc::PictureImageLayer::Create();
      RegisterContentsLayer(image_layer_.get());
    }
    image_layer_->SetImage(std::move(paint_image), matrix,
                           image_orientation.UsesWidthAsHeight());
    image_layer_->SetContentsOpaque(image->CurrentFrameKnownToBeOpaque());
    UpdateContentsRect();
  } else if (image_layer_) {
    UnregisterContentsLayer(image_layer_.get());
    image_layer_ = nullptr;
  }

  SetContentsTo(image_layer_.get(),
                /*prevent_contents_opaque_changes=*/true);
}

cc::Layer* GraphicsLayer::CcLayer() const {
  return layer_.get();
}

void GraphicsLayer::SetFilters(CompositorFilterOperations filters) {
  CcLayer()->SetFilters(filters.ReleaseCcFilterOperations());
}

void GraphicsLayer::SetBackdropFilters(CompositorFilterOperations filters) {
  CcLayer()->SetBackgroundFilters(filters.ReleaseCcFilterOperations());
}

void GraphicsLayer::SetStickyPositionConstraint(
    const cc::LayerStickyPositionConstraint& sticky_constraint) {
  layer_->SetStickyPositionConstraint(sticky_constraint);
}

void GraphicsLayer::SetFilterQuality(SkFilterQuality filter_quality) {
  if (image_layer_)
    image_layer_->SetNearestNeighbor(filter_quality == kNone_SkFilterQuality);
}

void GraphicsLayer::SetPaintingPhase(GraphicsLayerPaintingPhase phase) {
  if (painting_phase_ == phase)
    return;
  painting_phase_ = phase;
  SetNeedsDisplay();
}

void GraphicsLayer::AddLinkHighlight(LinkHighlight* link_highlight) {
  DCHECK(link_highlight && !link_highlights_.Contains(link_highlight));
  link_highlights_.push_back(link_highlight);
  link_highlight->Layer()->SetLayerClient(weak_ptr_factory_.GetWeakPtr());
  UpdateChildList();
}

void GraphicsLayer::RemoveLinkHighlight(LinkHighlight* link_highlight) {
  link_highlights_.EraseAt(link_highlights_.Find(link_highlight));
  UpdateChildList();
}

void GraphicsLayer::SetScrollableArea(ScrollableArea* scrollable_area) {
  if (scrollable_area_ == scrollable_area)
    return;
  scrollable_area_ = scrollable_area;
}

void GraphicsLayer::ScrollableAreaDisposed() {
  scrollable_area_.Clear();
}

std::unique_ptr<base::trace_event::TracedValue> GraphicsLayer::TakeDebugInfo(
    cc::Layer* layer) {
  auto traced_value = std::make_unique<base::trace_event::TracedValue>();

  traced_value->SetString(
      "layer_name", WTF::StringUTF8Adaptor(DebugName(layer)).AsStringPiece());

  traced_value->BeginArray("compositing_reasons");
  for (const char* description :
       CompositingReason::Descriptions(compositing_reasons_))
    traced_value->AppendString(description);
  traced_value->EndArray();

  traced_value->BeginArray("squashing_disallowed_reasons");
  for (const char* description :
       SquashingDisallowedReason::Descriptions(squashing_disallowed_reasons_))
    traced_value->AppendString(description);
  traced_value->EndArray();

  if (owner_node_id_)
    traced_value->SetInteger("owner_node", owner_node_id_);

  if (auto* tracking = GetRasterInvalidationTracking()) {
    tracking->AddToTracedValue(*traced_value);
    tracking->ClearInvalidations();
  }

  return traced_value;
}

void GraphicsLayer::DidChangeScrollbarsHiddenIfOverlay(bool hidden) {
  if (scrollable_area_)
    scrollable_area_->SetScrollbarsHiddenIfOverlay(hidden);
}

PaintController& GraphicsLayer::GetPaintController() const {
  CHECK(DrawsContent());
  if (!paint_controller_) {
    paint_controller_ = PaintController::Create();
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      paint_controller_->SetTracksRasterInvalidations(
          client_.IsTrackingRasterInvalidations());
    }
  }
  return *paint_controller_;
}

void GraphicsLayer::SetElementId(const CompositorElementId& id) {
  if (cc::Layer* layer = CcLayer())
    layer->SetElementId(id);
}

CompositorElementId GraphicsLayer::GetElementId() const {
  if (cc::Layer* layer = CcLayer())
    return layer->element_id();
  return CompositorElementId();
}

sk_sp<PaintRecord> GraphicsLayer::CapturePaintRecord() const {
  DCHECK(DrawsContent());

  if (client_.ShouldThrottleRendering())
    return sk_sp<PaintRecord>(new PaintRecord);

  FloatRect bounds(IntRect(IntPoint(0, 0), ExpandedIntSize(Size())));
  GraphicsContext graphics_context(GetPaintController());
  graphics_context.BeginRecording(bounds);
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(layer_state_) << "No layer state for GraphicsLayer: " << DebugName();
    GetPaintController().GetPaintArtifact().Replay(
        graphics_context, layer_state_->state, layer_state_->offset);
  } else {
    GetPaintController().GetPaintArtifact().Replay(graphics_context,
                                                   PropertyTreeState::Root());
  }
  return graphics_context.EndRecording();
}

void GraphicsLayer::SetLayerState(const PropertyTreeState& layer_state,
                                  const IntPoint& layer_offset) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  if (!layer_state_) {
    layer_state_ =
        std::make_unique<LayerState>(LayerState{layer_state, layer_offset});
    return;
  }
  layer_state_->state = layer_state;
  layer_state_->offset = layer_offset;
}

scoped_refptr<cc::DisplayItemList> GraphicsLayer::PaintContentsToDisplayList(
    PaintingControlSetting painting_control) {
  TRACE_EVENT0("blink,benchmark", "GraphicsLayer::PaintContents");

  PaintController& paint_controller = GetPaintController();
  paint_controller.SetDisplayItemConstructionIsDisabled(
      painting_control == DISPLAY_LIST_CONSTRUCTION_DISABLED);
  paint_controller.SetSubsequenceCachingIsDisabled(
      painting_control == SUBSEQUENCE_CACHING_DISABLED);

  if (painting_control == PARTIAL_INVALIDATION)
    client_.InvalidateTargetElementForTesting();

  // We also disable caching when Painting or Construction are disabled. In both
  // cases we would like to compare assuming the full cost of recording, not the
  // cost of re-using cached content.
  if (painting_control == DISPLAY_LIST_CACHING_DISABLED ||
      painting_control == DISPLAY_LIST_PAINTING_DISABLED ||
      painting_control == DISPLAY_LIST_CONSTRUCTION_DISABLED)
    paint_controller.InvalidateAll();

  GraphicsContext::DisabledMode disabled_mode =
      GraphicsContext::kNothingDisabled;
  if (painting_control == DISPLAY_LIST_PAINTING_DISABLED ||
      painting_control == DISPLAY_LIST_CONSTRUCTION_DISABLED)
    disabled_mode = GraphicsContext::kFullyDisabled;

  // Anything other than PAINTING_BEHAVIOR_NORMAL is for testing. In non-testing
  // scenarios, it is an error to call GraphicsLayer::Paint. Actual painting
  // occurs in LocalFrameView::PaintTree() which calls GraphicsLayer::Paint();
  // this method merely copies the painted output to the cc::DisplayItemList.
  if (painting_control != PAINTING_BEHAVIOR_NORMAL)
    Paint(nullptr, disabled_mode);

  auto display_list = base::MakeRefCounted<cc::DisplayItemList>();

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    DCHECK(layer_state_) << "No layer state for GraphicsLayer: " << DebugName();
    PaintChunksToCcLayer::ConvertInto(
        GetPaintController().PaintChunks(), layer_state_->state,
        gfx::Vector2dF(layer_state_->offset.X(), layer_state_->offset.Y()),
        VisualRectSubpixelOffset(),
        paint_controller.GetPaintArtifact().GetDisplayItemList(),
        *display_list);
  } else {
    paint_controller.GetPaintArtifact().AppendToDisplayItemList(
        FloatSize(OffsetFromLayoutObjectWithSubpixelAccumulation()),
        *display_list);
  }

  paint_controller.SetDisplayItemConstructionIsDisabled(false);
  paint_controller.SetSubsequenceCachingIsDisabled(false);

  display_list->Finalize();
  return display_list;
}

size_t GraphicsLayer::GetApproximateUnsharedMemoryUsage() const {
  size_t result = sizeof(*this);
  result += GetPaintController().ApproximateUnsharedMemoryUsage();
  if (raster_invalidator_)
    result += raster_invalidator_->ApproximateUnsharedMemoryUsage();
  return result;
}

// Subpixel offset for visual rects which excluded composited layer's subpixel
// accumulation during paint invalidation.
// See PaintInvalidator::ExcludeCompositedLayerSubpixelAccumulation().
FloatSize GraphicsLayer::VisualRectSubpixelOffset() const {
  if (GetCompositingReasons() & CompositingReason::kComboAllDirectReasons)
    return FloatSize(client_.SubpixelAccumulation());
  return FloatSize();
}

bool ScopedSetNeedsDisplayInRectForTrackingOnly::s_enabled_ = false;

}  // namespace blink

#if DCHECK_IS_ON()
void showGraphicsLayerTree(const blink::GraphicsLayer* layer) {
  if (!layer) {
    LOG(ERROR) << "Cannot showGraphicsLayerTree for (nil).";
    return;
  }

  String output = layer->GetLayerTreeAsTextForTesting(0xffffffff);
  LOG(ERROR) << output.Utf8().data();
}

void showGraphicsLayers(const blink::GraphicsLayer* layer) {
  if (!layer) {
    LOG(ERROR) << "Cannot showGraphicsLayers for (nil).";
    return;
  }

  String output = layer->GetLayerTreeAsTextForTesting(
      0xffffffff & ~blink::kOutputAsLayerTree);
  LOG(ERROR) << output.Utf8().data();
}
#endif
