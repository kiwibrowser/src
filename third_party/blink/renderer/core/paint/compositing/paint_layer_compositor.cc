/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/animation/document_animations.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/animation/element_animations.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_video.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/compositing_inputs_updater.h"
#include "third_party/blink/renderer/core/paint/compositing/compositing_layer_assigner.h"
#include "third_party/blink/renderer/core/paint/compositing/compositing_requirements_updater.h"
#include "third_party/blink/renderer/core/paint/compositing/graphics_layer_tree_builder.h"
#include "third_party/blink/renderer/core/paint/compositing/graphics_layer_updater.h"
#include "third_party/blink/renderer/core/paint/object_paint_invalidator.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

PaintLayerCompositor::PaintLayerCompositor(LayoutView& layout_view)
    : layout_view_(layout_view),
      compositing_reason_finder_(layout_view),
      pending_update_type_(kCompositingUpdateNone),
      has_accelerated_compositing_(true),
      compositing_(false),
      root_should_always_composite_dirty_(true),
      in_overlay_fullscreen_video_(false),
      root_layer_attachment_(kRootLayerUnattached) {
  UpdateAcceleratedCompositingSettings();
}

PaintLayerCompositor::~PaintLayerCompositor() {
  DCHECK_EQ(root_layer_attachment_, kRootLayerUnattached);
}

bool PaintLayerCompositor::InCompositingMode() const {
  // FIXME: This should assert that lifecycle is >= CompositingClean since
  // the last step of updateIfNeeded can set this bit to false.
  DCHECK(layout_view_.Layer()->IsAllowedToQueryCompositingState());
  return compositing_;
}

bool PaintLayerCompositor::StaleInCompositingMode() const {
  return compositing_;
}

void PaintLayerCompositor::SetCompositingModeEnabled(bool enable) {
  if (enable == compositing_)
    return;

  compositing_ = enable;

  if (compositing_)
    EnsureRootLayer();
  else
    DestroyRootLayer();

  LocalFrameView* view = layout_view_.GetFrameView();
  view->SetUsesCompositedScrolling(
      view->GetFrame().GetSettings()->GetPreferCompositingToLCDTextEnabled());

  // Schedule an update in the parent frame so the <iframe>'s layer in the owner
  // document matches the compositing state here.
  if (HTMLFrameOwnerElement* owner_element =
          layout_view_.GetDocument().LocalOwner())
    owner_element->SetNeedsCompositingUpdate();
}

void PaintLayerCompositor::EnableCompositingModeIfNeeded() {
  if (!root_should_always_composite_dirty_)
    return;

  root_should_always_composite_dirty_ = false;
  if (compositing_)
    return;

  if (RootShouldAlwaysComposite()) {
    // FIXME: Is this needed? It was added in
    // https://bugs.webkit.org/show_bug.cgi?id=26651.
    // No tests fail if it's deleted.
    SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
    SetCompositingModeEnabled(true);
  }
}

bool PaintLayerCompositor::RootShouldAlwaysComposite() const {
  if (!has_accelerated_compositing_)
    return false;
  return layout_view_.GetFrame()->IsLocalRoot() ||
         compositing_reason_finder_.RequiresCompositingForScrollableFrame();
}

void PaintLayerCompositor::UpdateAcceleratedCompositingSettings() {
  compositing_reason_finder_.UpdateTriggers();
  has_accelerated_compositing_ = layout_view_.GetDocument()
                                     .GetSettings()
                                     ->GetAcceleratedCompositingEnabled();
  root_should_always_composite_dirty_ = true;
  if (root_layer_attachment_ != kRootLayerUnattached)
    RootLayer()->SetNeedsCompositingInputsUpdate();
}

bool PaintLayerCompositor::PreferCompositingToLCDTextEnabled() const {
  return layout_view_.GetDocument()
      .GetSettings()
      ->GetPreferCompositingToLCDTextEnabled();
}

static LayoutVideo* FindFullscreenVideoLayoutObject(Document& document) {
  // Recursively find the document that is in fullscreen.
  Element* fullscreen_element = Fullscreen::FullscreenElementFrom(document);
  Document* content_document = &document;
  while (fullscreen_element && fullscreen_element->IsFrameOwnerElement()) {
    content_document =
        ToHTMLFrameOwnerElement(fullscreen_element)->contentDocument();
    if (!content_document)
      return nullptr;
    fullscreen_element = Fullscreen::FullscreenElementFrom(*content_document);
  }
  if (!IsHTMLVideoElement(fullscreen_element))
    return nullptr;
  LayoutObject* layout_object = fullscreen_element->GetLayoutObject();
  if (!layout_object)
    return nullptr;
  return ToLayoutVideo(layout_object);
}

void PaintLayerCompositor::UpdateIfNeededRecursive(
    DocumentLifecycle::LifecycleState target_state) {
  CompositingReasonsStats compositing_reasons_stats;
  UpdateIfNeededRecursiveInternal(target_state, compositing_reasons_stats);
  UMA_HISTOGRAM_CUSTOM_COUNTS("Blink.Compositing.LayerPromotionCount.Overlap",
                              compositing_reasons_stats.overlap_layers, 1, 100,
                              5);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Blink.Compositing.LayerPromotionCount.ActiveAnimation",
      compositing_reasons_stats.active_animation_layers, 1, 100, 5);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Blink.Compositing.LayerPromotionCount.AssumedOverlap",
      compositing_reasons_stats.assumed_overlap_layers, 1, 100, 5);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Blink.Compositing.LayerPromotionCount.IndirectComposited",
      compositing_reasons_stats.indirect_composited_layers, 1, 100, 5);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Blink.Compositing.LayerPromotionCount.TotalComposited",
      compositing_reasons_stats.total_composited_layers, 1, 1000, 10);
}

void PaintLayerCompositor::UpdateIfNeededRecursiveInternal(
    DocumentLifecycle::LifecycleState target_state,
    CompositingReasonsStats& compositing_reasons_stats) {
  DCHECK(target_state >= DocumentLifecycle::kCompositingInputsClean);

  if (layout_view_.GetFrameView()->ShouldThrottleRendering())
    return;

  LocalFrameView* view = layout_view_.GetFrameView();
  view->ResetNeedsForcedCompositingUpdate();

  for (Frame* child =
           layout_view_.GetFrameView()->GetFrame().Tree().FirstChild();
       child; child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    LocalFrame* local_frame = ToLocalFrame(child);
    // It's possible for trusted Pepper plugins to force hit testing in
    // situations where the frame tree is in an inconsistent state, such as in
    // the middle of frame detach.
    // TODO(bbudge) Remove this check when trusted Pepper plugins are gone.
    if (local_frame->GetDocument()->IsActive() &&
        local_frame->ContentLayoutObject()) {
      local_frame->ContentLayoutObject()
          ->Compositor()
          ->UpdateIfNeededRecursiveInternal(target_state,
                                            compositing_reasons_stats);
    }
  }

  TRACE_EVENT0("blink,benchmark",
               "PaintLayerCompositor::updateIfNeededRecursive");

  DCHECK(!layout_view_.NeedsLayout());

  ScriptForbiddenScope forbid_script;

  // FIXME: enableCompositingModeIfNeeded can trigger a
  // CompositingUpdateRebuildTree, which asserts that it's not
  // InCompositingUpdate.
  EnableCompositingModeIfNeeded();

  RootLayer()->UpdateDescendantDependentFlags();

  layout_view_.CommitPendingSelection();

  UpdateIfNeeded(target_state, compositing_reasons_stats);
  DCHECK(Lifecycle().GetState() == DocumentLifecycle::kCompositingInputsClean ||
         Lifecycle().GetState() == DocumentLifecycle::kCompositingClean);
  if (target_state == DocumentLifecycle::kCompositingInputsClean)
    return;

  // When printing a document, there is no use in updating composited animations
  // since we won't use the results.
  //
  // RuntimeEnabledFeatures::PrintBrowserEnabled is a mode which runs the
  // browser normally, but renders every page as if it were being printed.  See
  // crbug.com/667547
  if (!layout_view_.GetDocument().Printing() ||
      RuntimeEnabledFeatures::PrintBrowserEnabled()) {
    // Although BlinkGenPropertyTreesEnabled still uses PaintLayerCompositor to
    // generate the composited layer tree/list, it also has the SPv2 behavior of
    // removing layers that do not draw content. As such, we use the same path
    // as SPv2 for updating composited animations once we know the final set of
    // composited elements (see LocalFrameView::UpdateLifecyclePhasesInternal,
    // during kPaintClean).
    if (!RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled()) {
      base::Optional<CompositorElementIdSet> composited_element_ids;
      DocumentAnimations::UpdateAnimations(layout_view_.GetDocument(),
                                           DocumentLifecycle::kCompositingClean,
                                           composited_element_ids);
    }

    layout_view_.GetFrameView()
        ->GetScrollableArea()
        ->UpdateCompositorScrollAnimations();
    if (const LocalFrameView::ScrollableAreaSet* animating_scrollable_areas =
            layout_view_.GetFrameView()->AnimatingScrollableAreas()) {
      for (ScrollableArea* scrollable_area : *animating_scrollable_areas)
        scrollable_area->UpdateCompositorScrollAnimations();
    }
  }

#if DCHECK_IS_ON()
  DCHECK_EQ(Lifecycle().GetState(), DocumentLifecycle::kCompositingClean);
  AssertNoUnresolvedDirtyBits();
  for (Frame* child =
           layout_view_.GetFrameView()->GetFrame().Tree().FirstChild();
       child; child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    LocalFrame* local_frame = ToLocalFrame(child);
    if (local_frame->ShouldThrottleRendering() ||
        !local_frame->ContentLayoutObject())
      continue;
    local_frame->ContentLayoutObject()
        ->Compositor()
        ->AssertNoUnresolvedDirtyBits();
  }
#endif
}

void PaintLayerCompositor::SetNeedsCompositingUpdate(
    CompositingUpdateType update_type) {
  DCHECK_NE(update_type, kCompositingUpdateNone);
  pending_update_type_ = std::max(pending_update_type_, update_type);
  if (Page* page = GetPage())
    page->Animator().ScheduleVisualUpdate(layout_view_.GetFrame());
  Lifecycle().EnsureStateAtMost(DocumentLifecycle::kLayoutClean);
}

void PaintLayerCompositor::DidLayout() {
  // FIXME: Technically we only need to do this when the LocalFrameView's
  // isScrollable method would return a different value.
  root_should_always_composite_dirty_ = true;
  EnableCompositingModeIfNeeded();

  // FIXME: Rather than marking the entire LayoutView as dirty, we should
  // track which Layers moved during layout and only dirty those
  // specific Layers.
  RootLayer()->SetNeedsCompositingInputsUpdate();
}

#if DCHECK_IS_ON()

void PaintLayerCompositor::AssertNoUnresolvedDirtyBits() {
  DCHECK_EQ(pending_update_type_, kCompositingUpdateNone);
  DCHECK(!root_should_always_composite_dirty_);
}

#endif

void PaintLayerCompositor::ApplyOverlayFullscreenVideoAdjustmentIfNeeded() {
  in_overlay_fullscreen_video_ = false;
  GraphicsLayer* content_parent = ParentForContentLayers();
  if (!content_parent)
    return;

  bool is_local_root = layout_view_.GetFrame()->IsLocalRoot();
  LayoutVideo* video =
      FindFullscreenVideoLayoutObject(layout_view_.GetDocument());
  if (!video || !video->Layer()->HasCompositedLayerMapping() ||
      !video->VideoElement()->UsesOverlayFullscreenVideo()) {
    return;
  }

  GraphicsLayer* video_layer =
      video->Layer()->GetCompositedLayerMapping()->MainGraphicsLayer();

  // The fullscreen video has layer position equal to its enclosing frame's
  // scroll position because fullscreen container is fixed-positioned.
  // We should reset layer position here since we are going to reattach the
  // layer at the very top level.
  video_layer->SetPosition(IntPoint());

  // Only steal fullscreen video layer and clear all other layers if we are the
  // main frame.
  if (!is_local_root)
    return;

  content_parent->RemoveAllChildren();
  content_parent->AddChild(video_layer);

  in_overlay_fullscreen_video_ = true;
}

void PaintLayerCompositor::UpdateWithoutAcceleratedCompositing(
    CompositingUpdateType update_type) {
  DCHECK(!HasAcceleratedCompositing());

  if (update_type >= kCompositingUpdateAfterCompositingInputChange)
    CompositingInputsUpdater(RootLayer()).Update();

#if DCHECK_IS_ON()
  CompositingInputsUpdater::AssertNeedsCompositingInputsUpdateBitsCleared(
      RootLayer());
#endif
}

static void ForceRecomputeVisualRectsIncludingNonCompositingDescendants(
    LayoutObject& layout_object) {
  // We clear the previous visual rect as it's wrong (paint invalidation
  // container changed, ...). Forcing a full invalidation will make us recompute
  // it. Also we are not changing the previous position from our paint
  // invalidation container, which is fine as we want a full paint invalidation
  // anyway.
  layout_object.ClearPreviousVisualRects();

  for (LayoutObject* child = layout_object.SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (!child->IsPaintInvalidationContainer())
      ForceRecomputeVisualRectsIncludingNonCompositingDescendants(*child);
  }
}

GraphicsLayer* PaintLayerCompositor::ParentForContentLayers(
    GraphicsLayer* child_frame_parent_candidate) const {
  // Iframe content layers were connected by the parent frame using
  // AttachFrameContentLayersToIframeLayer. Return whatever candidate was given
  // to us as the child frame parent.
  if (!IsMainFrame())
    return child_frame_parent_candidate;

  // If this is a popup, don't hook into the VisualViewport layers.
  if (layout_view_.GetDocument().GetPage()->GetChromeClient().IsPopup())
    return nullptr;

  return GetVisualViewport().ScrollLayer();
}

void PaintLayerCompositor::UpdateIfNeeded(
    DocumentLifecycle::LifecycleState target_state,
    CompositingReasonsStats& compositing_reasons_stats) {
  DCHECK(target_state >= DocumentLifecycle::kCompositingInputsClean);

  Lifecycle().AdvanceTo(DocumentLifecycle::kInCompositingUpdate);

  if (pending_update_type_ < kCompositingUpdateAfterCompositingInputChange &&
      target_state == DocumentLifecycle::kCompositingInputsClean) {
    // The compositing inputs are already clean and that is our target state.
    // Early-exit here without clearing the pending update type since we haven't
    // handled e.g. geometry updates.
    Lifecycle().AdvanceTo(DocumentLifecycle::kCompositingInputsClean);
    return;
  }

  CompositingUpdateType update_type = pending_update_type_;
  pending_update_type_ = kCompositingUpdateNone;

  if (!HasAcceleratedCompositing()) {
    UpdateWithoutAcceleratedCompositing(update_type);
    Lifecycle().AdvanceTo(
        std::min(DocumentLifecycle::kCompositingClean, target_state));
    return;
  }

  if (update_type == kCompositingUpdateNone) {
    Lifecycle().AdvanceTo(
        std::min(DocumentLifecycle::kCompositingClean, target_state));
    return;
  }

  PaintLayer* update_root = RootLayer();

  Vector<PaintLayer*> layers_needing_paint_invalidation;

  if (update_type >= kCompositingUpdateAfterCompositingInputChange) {
    CompositingInputsUpdater(update_root).Update();

#if DCHECK_IS_ON()
    // FIXME: Move this check to the end of the compositing update.
    CompositingInputsUpdater::AssertNeedsCompositingInputsUpdateBitsCleared(
        update_root);
#endif

    // In the case where we only want to make compositing inputs clean, we
    // early-exit here. Because we have not handled the other implications of
    // |pending_update_type_| > kCompositingUpdateNone, we must restore the
    // pending update type for a future call.
    if (target_state == DocumentLifecycle::kCompositingInputsClean) {
      pending_update_type_ = update_type;
      Lifecycle().AdvanceTo(DocumentLifecycle::kCompositingInputsClean);
      return;
    }

    CompositingRequirementsUpdater(layout_view_, compositing_reason_finder_)
        .Update(update_root, compositing_reasons_stats);

    CompositingLayerAssigner layer_assigner(this);
    layer_assigner.Assign(update_root, layers_needing_paint_invalidation);

    bool layers_changed = layer_assigner.LayersChanged();

    {
      TRACE_EVENT0("blink",
                   "PaintLayerCompositor::updateAfterCompositingChange");
      if (const LocalFrameView::ScrollableAreaSet* scrollable_areas =
              layout_view_.GetFrameView()->ScrollableAreas()) {
        for (ScrollableArea* scrollable_area : *scrollable_areas)
          layers_changed |= scrollable_area->UpdateAfterCompositingChange();
      }
      layers_changed |=
          layout_view_.GetFrameView()->UpdateAfterCompositingChange();
    }

    if (layers_changed) {
      update_type = std::max(update_type, kCompositingUpdateRebuildTree);
      if (ScrollingCoordinator* scrolling_coordinator =
              GetScrollingCoordinator()) {
        LocalFrameView* frame_view = layout_view_.GetFrameView();
        scrolling_coordinator->NotifyGeometryChanged(frame_view);
      }
    }
  }

  GraphicsLayer* current_parent = nullptr;
  // Save off our current parent. We need this in subframes, because our
  // parent attached us to itself via AttachFrameContentLayersToIframeLayer().
  // However, if we're about to update our layer structure in
  // GraphicsLayerUpdater, we will sometimes remove our root graphics layer
  // from its parent. If there are no further tree updates, this means that
  // our root graphics layer will not be attached to anything. Below, we would
  // normally get the ParentForContentLayer to fix up this situation. However,
  // in RLS non-main frames don't have this parent. So, instead use this
  // saved-off parent.
  if (!IsMainFrame() && update_root->GetCompositedLayerMapping()) {
    current_parent = update_root->GetCompositedLayerMapping()
                         ->ChildForSuperlayers()
                         ->Parent();
  }

  GraphicsLayerUpdater updater;
  updater.Update(*update_root, layers_needing_paint_invalidation);

  if (updater.NeedsRebuildTree())
    update_type = std::max(update_type, kCompositingUpdateRebuildTree);

#if DCHECK_IS_ON()
  // FIXME: Move this check to the end of the compositing update.
  GraphicsLayerUpdater::AssertNeedsToUpdateGraphicsLayerBitsCleared(
      *update_root);
#endif

  if (update_type >= kCompositingUpdateRebuildTree) {
    GraphicsLayerVector child_list;
    {
      TRACE_EVENT0("blink", "GraphicsLayerTreeBuilder::rebuild");
      GraphicsLayerTreeBuilder().Rebuild(*update_root, child_list);
    }

    if (!child_list.IsEmpty()) {
      CHECK(compositing_);
      if (GraphicsLayer* content_parent =
              ParentForContentLayers(current_parent)) {
        content_parent->SetChildren(child_list);
      }
    }

    ApplyOverlayFullscreenVideoAdjustmentIfNeeded();
  }

  for (unsigned i = 0; i < layers_needing_paint_invalidation.size(); i++) {
    ForceRecomputeVisualRectsIncludingNonCompositingDescendants(
        layers_needing_paint_invalidation[i]->GetLayoutObject());
  }

  if (root_layer_attachment_ == kRootLayerPendingAttachViaChromeClient) {
    if (Page* page = layout_view_.GetFrame()->GetPage()) {
      page->GetChromeClient().AttachRootGraphicsLayer(RootGraphicsLayer(),
                                                      layout_view_.GetFrame());
      root_layer_attachment_ = kRootLayerAttachedViaChromeClient;
    }
  }

  // Inform the inspector that the layer tree has changed.
  if (IsMainFrame())
    probe::layerTreeDidChange(layout_view_.GetFrame());

  Lifecycle().AdvanceTo(DocumentLifecycle::kCompositingClean);
}

static void RestartAnimationOnCompositor(const LayoutObject& layout_object) {
  Node* node = layout_object.GetNode();
  ElementAnimations* element_animations =
      (node && node->IsElementNode()) ? ToElement(node)->GetElementAnimations()
                                      : nullptr;
  if (element_animations)
    element_animations->RestartAnimationOnCompositor();
}

bool PaintLayerCompositor::AllocateOrClearCompositedLayerMapping(
    PaintLayer* layer,
    const CompositingStateTransitionType composited_layer_update) {
  bool composited_layer_mapping_changed = false;

  // FIXME: It would be nice to directly use the layer's compositing reason,
  // but allocateOrClearCompositedLayerMapping also gets called without having
  // updated compositing requirements fully.
  switch (composited_layer_update) {
    case kAllocateOwnCompositedLayerMapping:
      DCHECK(!layer->HasCompositedLayerMapping());
      SetCompositingModeEnabled(true);

      // If we need to issue paint invalidations, do so before allocating the
      // compositedLayerMapping and clearing out the groupedMapping.
      PaintInvalidationOnCompositingChange(layer);

      // If this layer was previously squashed, we need to remove its reference
      // to a groupedMapping right away, so that computing paint invalidation
      // rects will know the layer's correct compositingState.
      // FIXME: do we need to also remove the layer from it's location in the
      // squashing list of its groupedMapping?  Need to create a test where a
      // squashed layer pops into compositing. And also to cover all other sorts
      // of compositingState transitions.
      layer->SetLostGroupedMapping(false);
      layer->SetGroupedMapping(
          nullptr, PaintLayer::kInvalidateLayerAndRemoveFromMapping);

      layer->EnsureCompositedLayerMapping();
      composited_layer_mapping_changed = true;

      RestartAnimationOnCompositor(layer->GetLayoutObject());

      // At this time, the ScrollingCoordinator only supports the top-level
      // frame.
      if (layer->IsRootLayer() && layout_view_.GetFrame()->IsLocalRoot()) {
        if (ScrollingCoordinator* scrolling_coordinator =
                GetScrollingCoordinator()) {
          scrolling_coordinator->FrameViewRootLayerDidChange(
              layout_view_.GetFrameView());
        }
      }
      break;
    case kRemoveOwnCompositedLayerMapping:
    // PutInSquashingLayer means you might have to remove the composited layer
    // mapping first.
    case kPutInSquashingLayer:
      if (layer->HasCompositedLayerMapping()) {
        layer->ClearCompositedLayerMapping();
        composited_layer_mapping_changed = true;
      }

      break;
    case kRemoveFromSquashingLayer:
    case kNoCompositingStateChange:
      // Do nothing.
      break;
  }

  if (!composited_layer_mapping_changed)
    return false;

  if (layer->GetLayoutObject().IsLayoutEmbeddedContent()) {
    PaintLayerCompositor* inner_compositor = FrameContentsCompositor(
        ToLayoutEmbeddedContent(layer->GetLayoutObject()));
    if (inner_compositor && inner_compositor->StaleInCompositingMode())
      inner_compositor->EnsureRootLayer();
  }

  layer->ClearClipRects(kPaintingClipRects);

  // If a fixed position layer gained/lost a compositedLayerMapping or the
  // reason not compositing it changed, the scrolling coordinator needs to
  // recalculate whether it can do fast scrolling.
  if (ScrollingCoordinator* scrolling_coordinator = GetScrollingCoordinator()) {
    scrolling_coordinator->FrameViewFixedObjectsDidChange(
        layout_view_.GetFrameView());
  }

  // Compositing state affects whether to create paint offset translation of
  // this layer, and amount of paint offset translation of descendants.
  layer->GetLayoutObject().SetNeedsPaintPropertyUpdate();

  return true;
}

void PaintLayerCompositor::PaintInvalidationOnCompositingChange(
    PaintLayer* layer) {
  // If the layoutObject is not attached yet, no need to issue paint
  // invalidations.
  if (&layer->GetLayoutObject() != &layout_view_ &&
      !layer->GetLayoutObject().Parent())
    return;

  // For querying Layer::compositingState()
  // Eager invalidation here is correct, since we are invalidating with respect
  // to the previous frame's compositing state when changing the compositing
  // backing of the layer.
  DisableCompositingQueryAsserts disabler;
  // We have to do immediate paint invalidation because compositing will change.
  DisablePaintInvalidationStateAsserts paint_invalidation_assertisabler;

  ObjectPaintInvalidator(layer->GetLayoutObject())
      .InvalidatePaintIncludingNonCompositingDescendants();
}

PaintLayerCompositor* PaintLayerCompositor::FrameContentsCompositor(
    LayoutEmbeddedContent& layout_object) {
  if (!layout_object.GetNode()->IsFrameOwnerElement())
    return nullptr;

  HTMLFrameOwnerElement* element =
      ToHTMLFrameOwnerElement(layout_object.GetNode());
  if (Document* content_document = element->contentDocument()) {
    if (auto* view = content_document->GetLayoutView())
      return view->Compositor();
  }
  return nullptr;
}

bool PaintLayerCompositor::AttachFrameContentLayersToIframeLayer(
    LayoutEmbeddedContent& layout_object) {
  PaintLayerCompositor* inner_compositor =
      FrameContentsCompositor(layout_object);
  if (!inner_compositor || !inner_compositor->StaleInCompositingMode() ||
      inner_compositor->root_layer_attachment_ !=
          kRootLayerAttachedViaEnclosingFrame)
    return false;

  PaintLayer* layer = layout_object.Layer();
  if (!layer->HasCompositedLayerMapping())
    return false;

  DisableCompositingQueryAsserts disabler;
  inner_compositor->RootLayer()->EnsureCompositedLayerMapping();
  layer->GetCompositedLayerMapping()->SetSublayers(
      GraphicsLayerVector(1, inner_compositor->RootGraphicsLayer()));
  return true;
}

static void FullyInvalidatePaintRecursive(PaintLayer* layer) {
  if (layer->GetCompositingState() == kPaintsIntoOwnBacking) {
    layer->GetCompositedLayerMapping()->SetContentsNeedDisplay();
    layer->GetCompositedLayerMapping()->SetSquashingContentsNeedDisplay();
  }

  for (PaintLayer* child = layer->FirstChild(); child;
       child = child->NextSibling())
    FullyInvalidatePaintRecursive(child);
}

void PaintLayerCompositor::FullyInvalidatePaint() {
  // We're walking all compositing layers and invalidating them, so there's
  // no need to have up-to-date compositing state.
  DisableCompositingQueryAsserts disabler;
  FullyInvalidatePaintRecursive(RootLayer());
}

PaintLayer* PaintLayerCompositor::RootLayer() const {
  return layout_view_.Layer();
}

GraphicsLayer* PaintLayerCompositor::RootGraphicsLayer() const {
  if (CompositedLayerMapping* clm = RootLayer()->GetCompositedLayerMapping())
    return clm->ChildForSuperlayers();
  return nullptr;
}

GraphicsLayer* PaintLayerCompositor::PaintRootGraphicsLayer() const {
  if (RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled()) {
    // Start painting at the inner viewport container layer which is an ancestor
    // of both the main contents layers and the scrollbar layers.
    if (IsMainFrame() && GetVisualViewport().ContainerLayer())
      return GetVisualViewport().ContainerLayer();

    return RootGraphicsLayer();
  }

  if (ParentForContentLayers() && ParentForContentLayers()->Children().size()) {
    DCHECK_EQ(ParentForContentLayers()->Children().size(), 1U);
    return ParentForContentLayers()->Children()[0];
  }

  return RootGraphicsLayer();
}

GraphicsLayer* PaintLayerCompositor::ScrollLayer() const {
  if (ScrollableArea* scrollable_area =
          layout_view_.GetFrameView()->GetScrollableArea())
    return scrollable_area->LayerForScrolling();
  return nullptr;
}

void PaintLayerCompositor::SetIsInWindow(bool is_in_window) {
  if (!StaleInCompositingMode())
    return;

  if (is_in_window) {
    if (root_layer_attachment_ != kRootLayerUnattached)
      return;

    AttachCompositorTimeline();
    AttachRootLayer();
  } else {
    if (root_layer_attachment_ == kRootLayerUnattached)
      return;

    DetachRootLayer();
    DetachCompositorTimeline();
  }
}

void PaintLayerCompositor::UpdatePotentialCompositingReasonsFromStyle(
    PaintLayer& layer) {
  layer.SetPotentialCompositingReasonsFromStyle(
      compositing_reason_finder_.PotentialCompositingReasonsFromStyle(
          layer.GetLayoutObject()));
}

bool PaintLayerCompositor::CanBeComposited(const PaintLayer* layer) const {
  LocalFrameView* frame_view = layer->GetLayoutObject().GetFrameView();
  // Elements within an invisible frame must not be composited because they are
  // not drawn.
  if (frame_view && !frame_view->IsVisible())
    return false;

  const bool has_compositor_animation =
      compositing_reason_finder_.CompositingReasonsForAnimation(
          *layer->GetLayoutObject().Style()) != CompositingReason::kNone;
  return has_accelerated_compositing_ &&
         (has_compositor_animation || !layer->SubtreeIsInvisible()) &&
         layer->IsSelfPaintingLayer() &&
         !layer->GetLayoutObject().IsLayoutFlowThread() &&
         // Don't composite <foreignObject> for the moment, to reduce
         // instances of the "fundamental compositing bug" breaking content.
         !layer->GetLayoutObject().IsSVGForeignObject();
}

// Return true if the given layer is a stacking context and has compositing
// child layers that it needs to clip, or is an embedded object with a border
// radius. In these cases we insert a clipping GraphicsLayer into the hierarchy
// between this layer and its children in the z-order hierarchy.
bool PaintLayerCompositor::ClipsCompositingDescendants(
    const PaintLayer* layer) const {
  if (!layer->HasCompositingDescendant())
    return false;
  if (!layer->GetLayoutObject().IsBox())
    return false;
  const LayoutBox& box = ToLayoutBox(layer->GetLayoutObject());
  return box.ShouldClipOverflow() || box.HasClip() ||
         (box.IsLayoutEmbeddedContent() && box.StyleRef().HasBorderRadius());
}

// If an element has composited negative z-index children, those children paint
// in front of the layer background, so we need an extra 'contents' layer for
// the foreground of the layer object.
bool PaintLayerCompositor::NeedsContentsCompositingLayer(
    const PaintLayer* layer) const {
  if (!layer->HasCompositingDescendant())
    return false;
  return layer->StackingNode()->HasNegativeZOrderList();
}

static void UpdateTrackingRasterInvalidationsRecursive(
    GraphicsLayer* graphics_layer) {
  if (!graphics_layer)
    return;

  graphics_layer->UpdateTrackingRasterInvalidations();

  for (size_t i = 0; i < graphics_layer->Children().size(); ++i)
    UpdateTrackingRasterInvalidationsRecursive(graphics_layer->Children()[i]);

  if (GraphicsLayer* mask_layer = graphics_layer->MaskLayer())
    UpdateTrackingRasterInvalidationsRecursive(mask_layer);

  if (GraphicsLayer* clipping_mask_layer =
          graphics_layer->ContentsClippingMaskLayer())
    UpdateTrackingRasterInvalidationsRecursive(clipping_mask_layer);
}

void PaintLayerCompositor::UpdateTrackingRasterInvalidations() {
#if DCHECK_IS_ON()
  DCHECK(Lifecycle().GetState() == DocumentLifecycle::kPaintClean ||
         layout_view_.GetFrameView()->ShouldThrottleRendering());
#endif

  if (GraphicsLayer* root_layer = RootGraphicsLayer())
    UpdateTrackingRasterInvalidationsRecursive(root_layer);
}

void PaintLayerCompositor::EnsureRootLayer() {
  if (root_layer_attachment_ != kRootLayerUnattached)
    return;

  if (IsMainFrame())
    GetVisualViewport().CreateLayerTree();

  AttachCompositorTimeline();
  AttachRootLayer();
}

void PaintLayerCompositor::DestroyRootLayer() {
  DetachRootLayer();
}

void PaintLayerCompositor::AttachRootLayer() {
  // In Slimming Paint v2, PaintArtifactCompositor is responsible for the root
  // layer.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  if (layout_view_.GetFrame()->IsLocalRoot()) {
    root_layer_attachment_ = kRootLayerPendingAttachViaChromeClient;
  } else {
    HTMLFrameOwnerElement* owner_element =
        layout_view_.GetDocument().LocalOwner();
    DCHECK(owner_element);
    // The layer will get hooked up via
    // CompositedLayerMapping::updateGraphicsLayerConfiguration() for the
    // frame's layoutObject in the parent document.
    owner_element->SetNeedsCompositingUpdate();
    root_layer_attachment_ = kRootLayerAttachedViaEnclosingFrame;
  }
}

void PaintLayerCompositor::DetachRootLayer() {
  if (root_layer_attachment_ == kRootLayerUnattached)
    return;

  switch (root_layer_attachment_) {
    case kRootLayerAttachedViaEnclosingFrame: {
      // The layer will get unhooked up via
      // CompositedLayerMapping::updateGraphicsLayerConfiguration() for the
      // frame's layoutObject in the parent document.
      if (HTMLFrameOwnerElement* owner_element =
              layout_view_.GetDocument().LocalOwner())
        owner_element->SetNeedsCompositingUpdate();
      break;
    }
    case kRootLayerAttachedViaChromeClient: {
      LocalFrame& frame = layout_view_.GetFrameView()->GetFrame();
      Page* page = frame.GetPage();
      if (!page)
        return;
      page->GetChromeClient().AttachRootGraphicsLayer(nullptr, &frame);
      break;
    }
    case kRootLayerPendingAttachViaChromeClient:
    case kRootLayerUnattached:
      break;
  }

  root_layer_attachment_ = kRootLayerUnattached;
}

void PaintLayerCompositor::AttachCompositorTimeline() {
  LocalFrame& frame = layout_view_.GetFrameView()->GetFrame();
  Page* page = frame.GetPage();
  if (!page || !frame.GetDocument())
    return;

  CompositorAnimationTimeline* compositor_timeline =
      frame.GetDocument()->Timeline().CompositorTimeline();
  if (compositor_timeline) {
    page->GetChromeClient().AttachCompositorAnimationTimeline(
        compositor_timeline, &frame);
  }
}

void PaintLayerCompositor::DetachCompositorTimeline() {
  LocalFrame& frame = layout_view_.GetFrameView()->GetFrame();
  Page* page = frame.GetPage();
  if (!page || !frame.GetDocument())
    return;

  CompositorAnimationTimeline* compositor_timeline =
      frame.GetDocument()->Timeline().CompositorTimeline();
  if (compositor_timeline) {
    page->GetChromeClient().DetachCompositorAnimationTimeline(
        compositor_timeline, &frame);
  }
}

ScrollingCoordinator* PaintLayerCompositor::GetScrollingCoordinator() const {
  if (Page* page = GetPage())
    return page->GetScrollingCoordinator();

  return nullptr;
}

Page* PaintLayerCompositor::GetPage() const {
  return layout_view_.GetFrameView()->GetFrame().GetPage();
}

DocumentLifecycle& PaintLayerCompositor::Lifecycle() const {
  return layout_view_.GetDocument().Lifecycle();
}

bool PaintLayerCompositor::IsMainFrame() const {
  return layout_view_.GetFrame()->IsMainFrame();
}

VisualViewport& PaintLayerCompositor::GetVisualViewport() const {
  return layout_view_.GetFrameView()->GetPage()->GetVisualViewport();
}

bool PaintLayerCompositor::IsRootScrollerAncestor() const {
  const TopDocumentRootScrollerController& global_rsc =
      layout_view_.GetDocument().GetPage()->GlobalRootScrollerController();
  PaintLayer* root_scroller_layer = global_rsc.RootScrollerPaintLayer();

  if (root_scroller_layer) {
    Frame* frame = root_scroller_layer->GetLayoutObject().GetFrame();
    while (frame) {
      if (frame->IsLocalFrame()) {
        PaintLayerCompositor* plc =
            ToLocalFrame(frame)->View()->GetLayoutView()->Compositor();
        if (plc == this)
          return true;
      }

      frame = frame->Tree().Parent();
    }
  }

  return false;
}

}  // namespace blink
