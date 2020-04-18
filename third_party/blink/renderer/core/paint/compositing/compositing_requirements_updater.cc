/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/paint/compositing/compositing_requirements_updater.h"

#include "base/macros.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node.h"
#include "third_party/blink/renderer/core/paint/paint_layer_stacking_node_iterator.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

class OverlapMapContainer {
 public:
  void Add(const IntRect& bounds) {
    layer_rects_.push_back(bounds);
    bounding_box_.Unite(bounds);
  }

  bool OverlapsLayers(const IntRect& bounds) const {
    // Checking with the bounding box will quickly reject cases when
    // layers are created for lists of items going in one direction and
    // never overlap with each other.
    if (!bounds.Intersects(bounding_box_))
      return false;
    for (unsigned i = 0; i < layer_rects_.size(); i++) {
      if (layer_rects_[i].Intersects(bounds))
        return true;
    }
    return false;
  }

  void Unite(const OverlapMapContainer& other_container) {
    layer_rects_.AppendVector(other_container.layer_rects_);
    bounding_box_.Unite(other_container.bounding_box_);
  }

 private:
  Vector<IntRect, 64> layer_rects_;
  IntRect bounding_box_;
};

struct OverlapMapContainers {
  OverlapMapContainer clipped;
  OverlapMapContainer unclipped;
};

class CompositingRequirementsUpdater::OverlapMap {

 public:
  OverlapMap() {
    // Begin by assuming the root layer will be composited so that there
    // is something on the stack. The root layer should also never get a
    // finishCurrentOverlapTestingContext() call.
    BeginNewOverlapTestingContext();
  }

  // Each rect added is marked as clipped or unclipped. clipped rects may
  // overlap only with other clipped rects, but unclipped rects may overlap
  // with anything.
  //
  // This is used to model composited overflow scrolling, where PaintLayers
  // within the scroller are not clipped for overlap testing, whereas
  // PaintLayers not within it are. This is necessary because PaintLayerClipper
  // is not smart enough to understand not to clip composited overflow clips,
  // but still clip otherwise.
  void Add(PaintLayer* layer, const IntRect& bounds, bool is_clipped) {
    DCHECK(!layer->IsRootLayer());
    if (bounds.IsEmpty())
      return;

    // Layers do not contribute to overlap immediately--instead, they will
    // contribute to overlap as soon as they have been recursively processed
    // and popped off the stack.
    DCHECK_GE(overlap_stack_.size(), 2ul);
    if (is_clipped)
      overlap_stack_[overlap_stack_.size() - 2].clipped.Add(bounds);
    else
      overlap_stack_[overlap_stack_.size() - 2].unclipped.Add(bounds);
  }

  bool OverlapsLayers(const IntRect& bounds, bool is_clipped) const {
    bool clipped_overlap = overlap_stack_.back().clipped.OverlapsLayers(bounds);
    if (is_clipped)
      return clipped_overlap;
    // Unclipped is allowed to overlap clipped, but not vice-versa.
    return clipped_overlap ||
           overlap_stack_.back().unclipped.OverlapsLayers(bounds);
  }

  void BeginNewOverlapTestingContext() {
    // This effectively creates a new "clean slate" for overlap state.
    // This is used when we know that a subtree or remaining set of
    // siblings does not need to check overlap with things behind it.
    overlap_stack_.Grow(overlap_stack_.size() + 1);
  }

  void FinishCurrentOverlapTestingContext() {
    // The overlap information on the top of the stack is still necessary
    // for checking overlap of any layers outside this context that may
    // overlap things from inside this context. Therefore, we must merge
    // the information from the top of the stack before popping the stack.
    //
    // FIXME: we may be able to avoid this deep copy by rearranging how
    //        overlapMap state is managed.
    overlap_stack_[overlap_stack_.size() - 2].clipped.Unite(
        overlap_stack_.back().clipped);
    overlap_stack_[overlap_stack_.size() - 2].unclipped.Unite(
        overlap_stack_.back().unclipped);
    overlap_stack_.pop_back();
  }

 private:
  Vector<OverlapMapContainers> overlap_stack_;
  DISALLOW_COPY_AND_ASSIGN(OverlapMap);
};

class CompositingRequirementsUpdater::RecursionData {
 public:
  explicit RecursionData(PaintLayer* compositing_ancestor)
      : compositing_ancestor_(compositing_ancestor),
        subtree_is_compositing_(false),
        has_unisolated_composited_blending_descendant_(false),
        testing_overlap_(true),
        has_composited_scrolling_ancestor_(false) {}

  PaintLayer* compositing_ancestor_;
  bool subtree_is_compositing_;
  bool has_unisolated_composited_blending_descendant_;
  bool testing_overlap_;
  bool has_composited_scrolling_ancestor_;
};

static bool RequiresCompositingOrSquashing(CompositingReasons reasons) {
#if DCHECK_IS_ON()
  bool fast_answer = reasons != CompositingReason::kNone;
  bool slow_answer = RequiresCompositing(reasons) || RequiresSquashing(reasons);
  DCHECK_EQ(slow_answer, fast_answer);
#endif
  return reasons != CompositingReason::kNone;
}

static CompositingReasons SubtreeReasonsForCompositing(
    const CompositingReasonFinder& compositing_reason_finder,
    PaintLayer* layer,
    bool has_composited_descendants,
    bool has3d_transformed_descendants) {
  CompositingReasons subtree_reasons = CompositingReason::kNone;
  if (!has_composited_descendants)
    return subtree_reasons;

  // When a layer has composited descendants, some effects, like 2d transforms,
  // filters, masks etc must be implemented via compositing so that they also
  // apply to those composited descendants.
  subtree_reasons |= layer->PotentialCompositingReasonsFromStyle() &
                     CompositingReason::kComboCompositedDescendants;

  if (layer->ShouldIsolateCompositedDescendants()) {
    DCHECK(layer->StackingNode()->IsStackingContext());
    subtree_reasons |= CompositingReason::kIsolateCompositedDescendants;
  }

  // FIXME: This should move into
  // CompositingReasonFinder::potentialCompositingReasonsFromStyle, but theres
  // a poor interaction with LayoutTextControlSingleLine, which sets this
  // hasOverflowClip directly.
  if (layer->GetLayoutObject().HasClipRelatedProperty())
    subtree_reasons |= CompositingReason::kClipsCompositingDescendants;

  // We ignore LCD text here because we are required to composite
  // scroll-dependant fixed position elements with composited descendants for
  // correctness - even if we lose LCD.
  //
  // TODO(smcgruer): Only composite fixed if needed (http://crbug.com/742213)
  const bool ignore_lcd_text = true;
  if (layer->GetLayoutObject().Style()->GetPosition() == EPosition::kFixed ||
      compositing_reason_finder.RequiresCompositingForScrollDependentPosition(
          layer, ignore_lcd_text)) {
    subtree_reasons |=
        CompositingReason::kPositionFixedOrStickyWithCompositedDescendants;
  }

  // A layer with preserve-3d or perspective only needs to be composited if
  // there are descendant layers that will be affected by the preserve-3d or
  // perspective.
  if (has3d_transformed_descendants) {
    subtree_reasons |= layer->PotentialCompositingReasonsFromStyle() &
                       CompositingReason::kCombo3DDescendants;
  }

  return subtree_reasons;
}

CompositingRequirementsUpdater::CompositingRequirementsUpdater(
    LayoutView& layout_view,
    CompositingReasonFinder& compositing_reason_finder)
    : layout_view_(layout_view),
      compositing_reason_finder_(compositing_reason_finder) {}

CompositingRequirementsUpdater::~CompositingRequirementsUpdater() = default;

void CompositingRequirementsUpdater::Update(
    PaintLayer* root,
    CompositingReasonsStats& compositing_reasons_stats) {
  TRACE_EVENT0("blink", "CompositingRequirementsUpdater::updateRecursive");

  // Go through the layers in presentation order, so that we can compute which
  // Layers need compositing layers.
  // FIXME: we could maybe do this and the hierarchy update in one pass, but the
  // parenting logic would be more complex.
  RecursionData recursion_data(root);
  OverlapMap overlap_test_request_map;
  bool saw3d_transform = false;

  // FIXME: Passing these unclippedDescendants down and keeping track
  // of them dynamically, we are requiring a full tree walk. This
  // should be removed as soon as proper overlap testing based on
  // scrolling and animation bounds is implemented (crbug.com/252472).
  Vector<PaintLayer*> unclipped_descendants;
  IntRect absolute_descendant_bounding_box;
  UpdateRecursive(nullptr, root, overlap_test_request_map, recursion_data,
                  saw3d_transform, unclipped_descendants,
                  absolute_descendant_bounding_box, compositing_reasons_stats);
}

void CompositingRequirementsUpdater::MaybeEnableCompositedScrolling(
    PaintLayer* layer,
    CompositingReasons& reasons) {
  // Add CompositingReasonOverflowScrollingTouch for layers that do not
  // already have it but need it.
  // Note that m_compositingReasonFinder.directReasons(layer) already includes
  // CompositingReasonOverflowScrollingTouch for anything that has
  // layer->needsCompositedScrolling() true. That is, for cases where we
  // explicitly decide not to have LCD text or cases where the layer will
  // still support LCD text even if the layer is composited.
  if (reasons && layer->ScrollsOverflow() &&
      !layer->NeedsCompositedScrolling()) {
    // We can get here for a scroller that will be composited for some other
    // reason and hence will already use grayscale AA text. We recheck for
    // needsCompositedScrolling ignoring LCD to correctly add the
    // CompositingReasonOverflowScrollingTouch reason to layers that can
    // support it with grayscale AA text.
    layer->GetScrollableArea()->UpdateNeedsCompositedScrolling(true);
    if (layer->NeedsCompositedScrolling())
      reasons |= CompositingReason::kOverflowScrollingTouch;
  }
}

void CompositingRequirementsUpdater::UpdateRecursive(
    PaintLayer* ancestor_layer,
    PaintLayer* layer,
    OverlapMap& overlap_map,
    RecursionData& current_recursion_data,
    bool& descendant_has3d_transform,
    Vector<PaintLayer*>& unclipped_descendants,
    IntRect& absolute_descendant_bounding_box,
    CompositingReasonsStats& compositing_reasons_stats) {
  PaintLayerCompositor* compositor = layout_view_.Compositor();

  layer->StackingNode()->UpdateLayerListsIfNeeded();

  CompositingReasons reasons_to_composite = CompositingReason::kNone;
  CompositingReasons direct_reasons = CompositingReason::kNone;

  // Video is special. It's the only PaintLayer type that can both have
  // PaintLayer children and whose children can't use its backing to render
  // into. These children (the controls) always need to be promoted into their
  // own layers to draw on top of the accelerated video.
  if (current_recursion_data.compositing_ancestor_ &&
      current_recursion_data.compositing_ancestor_->GetLayoutObject().IsVideo())
    direct_reasons |= CompositingReason::kVideoOverlay;

  bool has_composited_scrolling_ancestor =
      layer->AncestorScrollingLayer() &&
      (compositing_reason_finder_.DirectReasons(layer->AncestorScrollingLayer(),
                                                false) &
       CompositingReason::kOverflowScrollingTouch);

  bool use_clipped_bounding_rect =
      !has_composited_scrolling_ancestor ||
      layer->AncestorScrollingLayer()->IsRootLayer();

  // We have to promote the sticky element to work around the bug
  // (https://crbug.com/698358) of not being able to invalidate the ancestor
  // after updating the sticky layer position.
  // TODO(yigu): We should check if we have already lost lcd text. This
  // would require tracking if we think the current compositing ancestor
  // layer meets the requirements (i.e. opaque, integer transform, etc).
  const bool moves_with_respect_to_compositing_ancestor =
      layer->SticksToScroller() &&
      !current_recursion_data.compositing_ancestor_->IsRootLayer();
  // TODO(chrishtr): use |hasCompositedScrollingAncestor| instead.
  const bool ignore_lcd_text =
      current_recursion_data.has_composited_scrolling_ancestor_ ||
      moves_with_respect_to_compositing_ancestor;
  direct_reasons |=
      compositing_reason_finder_.DirectReasons(layer, ignore_lcd_text);

  bool can_be_composited = compositor->CanBeComposited(layer);
  if (can_be_composited) {
    reasons_to_composite |= direct_reasons;

    if (layer->IsRootLayer() && compositor->RootShouldAlwaysComposite())
      reasons_to_composite |= CompositingReason::kRoot;

    MaybeEnableCompositedScrolling(layer, reasons_to_composite);
  }

  if ((reasons_to_composite & CompositingReason::kOverflowScrollingTouch) &&
      !layer->IsRootLayer())
    current_recursion_data.has_composited_scrolling_ancestor_ = true;

  // Next, accumulate reasons related to overlap.
  // If overlap testing is used, this reason will be overridden. If overlap
  // testing is not used, we must assume we overlap if there is anything
  // composited behind us in paint-order.
  CompositingReasons overlap_compositing_reason =
      current_recursion_data.subtree_is_compositing_
          ? CompositingReason::kAssumedOverlap
          : CompositingReason::kNone;

  // TODO(chrishtr): use |hasCompositedScrollingAncestor| instead.
  if (current_recursion_data.has_composited_scrolling_ancestor_) {
    Vector<size_t> unclipped_descendants_to_remove;
    for (size_t i = 0; i < unclipped_descendants.size(); i++) {
      PaintLayer* unclipped_descendant = unclipped_descendants.at(i);
      // If we've reached the containing block of one of the unclipped
      // descendants, that element is no longer relevant to whether or not we
      // should opt in. Unfortunately we can't easily remove from the list
      // while we're iterating, so we have to store it for later removal.
      if (unclipped_descendant->GetLayoutObject().ContainingBlock() ==
          &layer->GetLayoutObject()) {
        unclipped_descendants_to_remove.push_back(i);
        continue;
      }
      if (layer->ScrollsWithRespectTo(unclipped_descendant))
        reasons_to_composite |= CompositingReason::kAssumedOverlap;
    }

    // Remove irrelevant unclipped descendants in reverse order so our stored
    // indices remain valid.
    for (size_t i = 0; i < unclipped_descendants_to_remove.size(); i++) {
      unclipped_descendants.EraseAt(unclipped_descendants_to_remove.at(
          unclipped_descendants_to_remove.size() - i - 1));
    }

    if (reasons_to_composite & CompositingReason::kOutOfFlowClipping) {
      // TODO(schenney): We only need to promote when the clipParent is not a
      // descendant of the ancestor scroller, which we do not check for here.
      // Hence we might be promoting needlessly.
      unclipped_descendants.push_back(layer);
    }
  }

  IntRect abs_bounds = use_clipped_bounding_rect
                           ? layer->ClippedAbsoluteBoundingBox()
                           : layer->UnclippedAbsoluteBoundingBox();
  PaintLayer* root_layer = layout_view_.Layer();
  // |abs_bounds| does not include root scroller offset. For the purposes
  // of overlap, this only matters for fixed-position objects, and their
  // relative position to other elements. Therefore, it's still correct to,
  // instead of adding scroll to all non-fixed elements, add a reverse scroll
  // to ones that are fixed.
  if (root_layer->GetScrollableArea() &&
      !layer->IsAffectedByScrollOf(root_layer)) {
    abs_bounds.Move(
        RoundedIntSize(root_layer->GetScrollableArea()->GetScrollOffset()));
  }

  absolute_descendant_bounding_box = abs_bounds;
  if (current_recursion_data.testing_overlap_ &&
      !RequiresCompositingOrSquashing(direct_reasons)) {
    bool overlaps =
        overlap_map.OverlapsLayers(abs_bounds, use_clipped_bounding_rect);
    overlap_compositing_reason =
        overlaps ? CompositingReason::kOverlap : CompositingReason::kNone;
  }

  reasons_to_composite |= overlap_compositing_reason;

  // The children of this layer don't need to composite, unless there is
  // a compositing layer among them, so start by inheriting the compositing
  // ancestor with m_subtreeIsCompositing set to false.
  RecursionData child_recursion_data = current_recursion_data;
  child_recursion_data.subtree_is_compositing_ = false;

  bool will_be_composited_or_squashed =
      can_be_composited && RequiresCompositingOrSquashing(reasons_to_composite);
  if (will_be_composited_or_squashed) {
    // This layer now acts as the ancestor for child layers.
    child_recursion_data.compositing_ancestor_ = layer;

    // Here we know that all children and the layer's own contents can blindly
    // paint into this layer's backing, until a descendant is composited. So, we
    // don't need to check for overlap with anything behind this layer.
    overlap_map.BeginNewOverlapTestingContext();
    // This layer is going to be composited, so children can safely ignore the
    // fact that there's an animation running behind this layer, meaning they
    // can rely on the overlap map testing again.
    child_recursion_data.testing_overlap_ = true;
  }

#if DCHECK_IS_ON()
  LayerListMutationDetector mutation_checker(layer->StackingNode());
#endif

  bool any_descendant_has3d_transform = false;
  bool will_have_foreground_layer = false;

  if (layer->StackingNode()->IsStackingContext()) {
    PaintLayerStackingNodeIterator iterator(*layer->StackingNode(),
                                            kNegativeZOrderChildren);
    while (PaintLayerStackingNode* cur_node = iterator.Next()) {
      IntRect absolute_child_descendant_bounding_box;
      UpdateRecursive(
          layer, cur_node->Layer(), overlap_map, child_recursion_data,
          any_descendant_has3d_transform, unclipped_descendants,
          absolute_child_descendant_bounding_box, compositing_reasons_stats);
      absolute_descendant_bounding_box.Unite(
          absolute_child_descendant_bounding_box);

      // If we have to make a layer for this child, make one now so we can have
      // a contents layer (since we need to ensure that the -ve z-order child
      // renders underneath our contents).
      if (child_recursion_data.subtree_is_compositing_) {
        reasons_to_composite |= CompositingReason::kNegativeZIndexChildren;

        if (!will_be_composited_or_squashed) {
          // make layer compositing
          child_recursion_data.compositing_ancestor_ = layer;
          overlap_map.BeginNewOverlapTestingContext();
          will_be_composited_or_squashed = true;
          will_have_foreground_layer = true;

          // FIXME: temporary solution for the first negative z-index composited
          // child: re-compute the absBounds for the child so that we can add
          // the negative z-index child's bounds to the new overlap context.
          overlap_map.BeginNewOverlapTestingContext();
          overlap_map.Add(cur_node->Layer(),
                          cur_node->Layer()->ClippedAbsoluteBoundingBox(),
                          true);
          overlap_map.FinishCurrentOverlapTestingContext();
        }
      }
    }
  }

  if (will_have_foreground_layer) {
    DCHECK(will_be_composited_or_squashed);
    // A foreground layer effectively is a new backing for all subsequent
    // children, so we don't need to test for overlap with anything behind this.
    // So, we can finish the previous context that was accumulating rects for
    // the negative z-index children, and start with a fresh new empty context.
    overlap_map.FinishCurrentOverlapTestingContext();
    overlap_map.BeginNewOverlapTestingContext();
    // This layer is going to be composited, so children can safely ignore the
    // fact that there's an animation running behind this layer, meaning they
    // can rely on the overlap map testing again.
    child_recursion_data.testing_overlap_ = true;
  }

  PaintLayerStackingNodeIterator iterator(
      *layer->StackingNode(), kNormalFlowChildren | kPositiveZOrderChildren);
  while (PaintLayerStackingNode* cur_node = iterator.Next()) {
    IntRect absolute_child_descendant_bounding_box;
    UpdateRecursive(layer, cur_node->Layer(), overlap_map, child_recursion_data,
                    any_descendant_has3d_transform, unclipped_descendants,
                    absolute_child_descendant_bounding_box,
                    compositing_reasons_stats);
    absolute_descendant_bounding_box.Unite(
        absolute_child_descendant_bounding_box);
  }

  // Now that the subtree has been traversed, we can check for compositing
  // reasons that depended on the state of the subtree.

  if (layer->StackingNode()->IsStackingContext()) {
    layer->SetShouldIsolateCompositedDescendants(
        child_recursion_data.has_unisolated_composited_blending_descendant_);
  } else {
    layer->SetShouldIsolateCompositedDescendants(false);
    current_recursion_data.has_unisolated_composited_blending_descendant_ =
        child_recursion_data.has_unisolated_composited_blending_descendant_;
  }

  // Embedded objects treat the embedded document as a child for the purposes
  // of composited layer decisions. Look into the embedded document to determine
  // if it is composited.
  bool contains_composited_iframe =
      layer->GetLayoutObject().IsLayoutEmbeddedContent() &&
      ToLayoutEmbeddedContent(layer->GetLayoutObject())
          .RequiresAcceleratedCompositing();

  // Subsequent layers in the parent's stacking context may also need to
  // composite.
  if (child_recursion_data.subtree_is_compositing_)
    current_recursion_data.subtree_is_compositing_ = true;

  // Set the flag to say that this SC has compositing children.
  layer->SetHasCompositingDescendant(
      child_recursion_data.subtree_is_compositing_ ||
      contains_composited_iframe);

  if (layer->IsRootLayer()) {
    // The root layer needs to be composited if anything else in the tree is
    // composited.  Otherwise, we can disable compositing entirely.
    if (child_recursion_data.subtree_is_compositing_ ||
        RequiresCompositingOrSquashing(reasons_to_composite) ||
        compositor->RootShouldAlwaysComposite()) {
      reasons_to_composite |= CompositingReason::kRoot;
      current_recursion_data.subtree_is_compositing_ = true;
      // Try again to enable composited scrolling if root became composited due
      // to subtree_is_compositing_ or overlap.
      // TODO(skobes): At this point we've already done recursion to descendant
      // layers, possibly with has_composited_scrolling_ancestor_ == false.
      // We should refactor overlap testing etc. to be independent of having
      // composited scrolling ancestors. See crbug.com/777672, crbug.com/782991.
      MaybeEnableCompositedScrolling(layer, reasons_to_composite);
    } else {
      compositor->SetCompositingModeEnabled(false);
      reasons_to_composite = CompositingReason::kNone;
    }
  } else {
    // All layers (even ones that aren't being composited) need to get added to
    // the overlap map. Layers that are not separately composited will paint
    // into their compositing ancestor's backing, and so are still considered
    // for overlap.
    if (child_recursion_data.compositing_ancestor_ &&
        !child_recursion_data.compositing_ancestor_->IsRootLayer())
      overlap_map.Add(layer, abs_bounds, use_clipped_bounding_rect);

    // Now check for reasons to become composited that depend on the state of
    // descendant layers.
    CompositingReasons subtree_compositing_reasons =
        SubtreeReasonsForCompositing(
            compositing_reason_finder_, layer,
            child_recursion_data.subtree_is_compositing_,
            any_descendant_has3d_transform);
    reasons_to_composite |= subtree_compositing_reasons;
    if (!will_be_composited_or_squashed && can_be_composited &&
        RequiresCompositingOrSquashing(subtree_compositing_reasons)) {
      child_recursion_data.compositing_ancestor_ = layer;
      // FIXME: this context push is effectively a no-op but needs to exist for
      // now, because the code is designed to push overlap information to the
      // second-from-top context of the stack.
      overlap_map.BeginNewOverlapTestingContext();
      overlap_map.Add(layer, absolute_descendant_bounding_box,
                      use_clipped_bounding_rect);
      will_be_composited_or_squashed = true;
    }

    if (will_be_composited_or_squashed) {
      reasons_to_composite |= layer->PotentialCompositingReasonsFromStyle() &
                              CompositingReason::kInlineTransform;
    }

    if (will_be_composited_or_squashed &&
        layer->GetLayoutObject().Style()->HasBlendMode()) {
      current_recursion_data.has_unisolated_composited_blending_descendant_ =
          true;
    }

    // Tell the parent it has compositing descendants.
    if (will_be_composited_or_squashed)
      current_recursion_data.subtree_is_compositing_ = true;

    // Turn overlap testing off for later layers if it's already off, or if we
    // have an animating transform.  Note that if the layer clips its
    // descendants, there's no reason to propagate the child animation to the
    // parent layers. That's because we know for sure the animation is contained
    // inside the clipping rectangle, which is already added to the overlap map.
    bool is_composited_clipping_layer =
        can_be_composited && (reasons_to_composite &
                              CompositingReason::kClipsCompositingDescendants);
    bool is_composited_with_inline_transform =
        reasons_to_composite & CompositingReason::kInlineTransform;
    if ((!child_recursion_data.testing_overlap_ &&
         !is_composited_clipping_layer) ||
        layer->GetLayoutObject().Style()->HasCurrentTransformAnimation() ||
        is_composited_with_inline_transform)
      current_recursion_data.testing_overlap_ = false;

    if (child_recursion_data.compositing_ancestor_ == layer)
      overlap_map.FinishCurrentOverlapTestingContext();

    descendant_has3d_transform |=
        any_descendant_has3d_transform || layer->Has3DTransform();
  }

  // At this point we have finished collecting all reasons to composite this
  // layer.
  layer->SetCompositingReasons(reasons_to_composite);
  if (reasons_to_composite & CompositingReason::kOverlap)
    compositing_reasons_stats.overlap_layers++;
  if (reasons_to_composite & CompositingReason::kComboActiveAnimation)
    compositing_reasons_stats.active_animation_layers++;
  if (reasons_to_composite & CompositingReason::kAssumedOverlap)
    compositing_reasons_stats.assumed_overlap_layers++;
  if (!(reasons_to_composite & CompositingReason::kComboAllDirectReasons))
    compositing_reasons_stats.indirect_composited_layers++;
  if (reasons_to_composite != CompositingReason::kNone)
    compositing_reasons_stats.total_composited_layers++;
}

}  // namespace blink
