// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_invalidator.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/layout_table.h"
#include "third_party/blink/renderer/core/layout/layout_table_section.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/paint/clip_path_clipper.h"
#include "third_party/blink/renderer/core/paint/find_paint_offset_and_visual_rect_needing_update.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/paint/pre_paint_tree_walk.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"

namespace blink {

template <typename Rect>
static LayoutRect SlowMapToVisualRectInAncestorSpace(
    const LayoutObject& object,
    const LayoutBoxModelObject& ancestor,
    const Rect& rect) {
  if (object.IsSVGChild()) {
    LayoutRect result;
    SVGLayoutSupport::MapToVisualRectInAncestorSpace(object, &ancestor,
                                                     FloatRect(rect), result);
    return result;
  }

  LayoutRect result(rect);
  if (object.IsLayoutView()) {
    ToLayoutView(object).MapToVisualRectInAncestorSpace(
        &ancestor, result, kInputIsInFrameCoordinates, kDefaultVisualRectFlags);
  } else {
    object.MapToVisualRectInAncestorSpace(&ancestor, result);
  }
  return result;
}

// If needed, exclude composited layer's subpixel accumulation to avoid full
// layer raster invalidations during animation with subpixels.
// See crbug.com/833083 for details.
template <typename Rect, typename Point>
void PaintInvalidator::ExcludeCompositedLayerSubpixelAccumulation(
    const LayoutObject& object,
    const PaintInvalidatorContext& context,
    Rect& rect) {
  // TODO(wangxianzhu): How to handle sub-pixel location animation for SPv2?
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  // One of the following conditions happened in crbug.com/837226.
  if (!context.paint_invalidation_container ||
      !context.paint_invalidation_container->FirstFragment()
           .HasLocalBorderBoxProperties() ||
      !context.tree_builder_context_)
    return;

  if (!(context.paint_invalidation_container->Layer()->GetCompositingReasons() &
        CompositingReason::kComboAllDirectReasons))
    return;

  if (object != context.paint_invalidation_container &&
      context.paint_invalidation_container->FirstFragment()
              .PostScrollTranslation() !=
          context.tree_builder_context_->current.transform) {
    // Subpixel accumulation doesn't propagate through non-translation
    // transforms. Also skip all transforms, to avoid the runtime cost of
    // verifying whether the transform is a translation.
    return;
  }

  // Exclude the subpixel accumulation so that the paint invalidator won't
  // see changed visual rects during composited animation with subpixels, to
  // avoid full layer invalidation. The subpixel accumulation will be added
  // back in ChunkToLayerMapper::AdjustVisualRectBySubpixelOffset(). Should
  // make sure the code is synced.
  // TODO(wangxianzhu): Avoid exposing subpixel accumulation to platform code.
  rect.MoveBy(Point(LayoutPoint(
      -context.paint_invalidation_container->Layer()->SubpixelAccumulation())));
}

// TODO(wangxianzhu): Combine this into
// PaintInvalidator::mapLocalRectToBacking() when removing
// PaintInvalidationState.
// This function is templatized to avoid FloatRect<->LayoutRect conversions
// which affect performance.
template <typename Rect, typename Point>
LayoutRect PaintInvalidator::MapLocalRectToVisualRectInBacking(
    const LayoutObject& object,
    const Rect& local_rect,
    const PaintInvalidatorContext& context,
    bool disable_flip) {
  DCHECK(context.NeedsVisualRectUpdate(object));
  if (local_rect.IsEmpty())
    return LayoutRect();

  bool is_svg_child = object.IsSVGChild();

  // TODO(wkorman): The flip below is required because local visual rects are
  // currently in "physical coordinates with flipped block-flow direction"
  // (see LayoutBoxModelObject.h) but we need them to be in physical
  // coordinates.
  Rect rect = local_rect;
  // Writing-mode flipping doesn't apply to non-root SVG.
  if (!is_svg_child) {
    if (!disable_flip) {
      if (object.IsBox()) {
        ToLayoutBox(object).FlipForWritingMode(rect);
      } else if (!(context.subtree_flags &
                   PaintInvalidatorContext::kSubtreeSlowPathRect)) {
        // For SPv2 and the GeometryMapper path, we also need to convert the
        // rect for non-boxes into physical coordinates before applying paint
        // offset. (Otherwise we'll call mapToVisualrectInAncestorSpace() which
        // requires physical coordinates for boxes, but "physical coordinates
        // with flipped block-flow direction" for non-boxes for which we don't
        // need to flip.)
        // TODO(wangxianzhu): Avoid containingBlock().
        object.ContainingBlock()->FlipForWritingMode(rect);
      }
    }

    // Unite visual rect with clip path bounding rect.
    // It is because the clip path display items are owned by the layout object
    // who has the clip path, and uses its visual rect as bounding rect too.
    // Usually it is done at layout object level and included as a part of
    // local visual overflow, but clip-path can be a reference to SVG, and we
    // have to wait until pre-paint to ensure clean layout.
    // Note: SVG children don't need this adjustment because their visual
    // overflow rects are already adjusted by clip path.
    if (base::Optional<FloatRect> clip_path_bounding_box =
            ClipPathClipper::LocalClipPathBoundingBox(object)) {
      Rect box(EnclosingIntRect(*clip_path_bounding_box));
      rect.Unite(box);
    }
  }

  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
    // In SPv175, visual rects are in the space of their local transform node.
    // For SVG, the input rect is in local SVG coordinates in which paint
    // offset doesn't apply.
    if (!is_svg_child)
      rect.MoveBy(Point(context.fragment_data->PaintOffset()));
    ExcludeCompositedLayerSubpixelAccumulation<Rect, Point>(object, context,
                                                            rect);
    // Use EnclosingIntRect to ensure the final visual rect will cover the rect
    // in source coordinates no matter if the painting will snap to pixels.
    return LayoutRect(EnclosingIntRect(rect));
  }

  LayoutRect result;
  if (context.subtree_flags & PaintInvalidatorContext::kSubtreeSlowPathRect) {
    result = SlowMapToVisualRectInAncestorSpace(
        object, *context.paint_invalidation_container, rect);
  } else if (object == context.paint_invalidation_container) {
    result = LayoutRect(rect);
  } else {
    // For non-root SVG, the input rect is in local SVG coordinates in which
    // paint offset doesn't apply.
    if (!is_svg_child)
      rect.MoveBy(Point(context.fragment_data->PaintOffset()));

    auto container_contents_properties =
        context.paint_invalidation_container->FirstFragment()
            .ContentsProperties();
    DCHECK(
        !context.paint_invalidation_container->FirstFragment().NextFragment());
    if (context.tree_builder_context_->current.transform ==
            container_contents_properties.Transform() &&
        context.tree_builder_context_->current.clip ==
            container_contents_properties.Clip() &&
        context.tree_builder_context_->current_effect ==
            container_contents_properties.Effect()) {
      result = LayoutRect(rect);
    } else {
      // Use enclosingIntRect to ensure the final visual rect will cover the
      // rect in source coordinates no matter if the painting will use pixel
      // snapping, when transforms are applied. If there is no transform,
      // enclosingIntRect is applied in the last step of paint invalidation
      // (see CompositedLayerMapping::setContentsNeedDisplayInRect()).
      if (!is_svg_child && context.tree_builder_context_->current.transform !=
                               container_contents_properties.Transform())
        rect = Rect(EnclosingIntRect(rect));

      PropertyTreeState current_tree_state(
          context.tree_builder_context_->current.transform,
          context.tree_builder_context_->current.clip,
          context.tree_builder_context_->current_effect);

      FloatClipRect float_rect((FloatRect(rect)));

      GeometryMapper::LocalToAncestorVisualRect(
          current_tree_state, container_contents_properties, float_rect);
      result = LayoutRect(float_rect.Rect());
    }

    // Convert the result to the paint invalidation container's contents space.
    // If the paint invalidation container has a transform node associated
    // with it (due to scroll or transform), then its PaintOffset
    // must be zero or equal to its subpixel accumulation, since in all
    // such cases we allocate a paint offset translation transform.
    result.MoveBy(
        -context.paint_invalidation_container->FirstFragment().PaintOffset());
  }

  if (!result.IsEmpty())
    result.Inflate(object.VisualRectOutsetForRasterEffects());

  PaintLayer::MapRectInPaintInvalidationContainerToBacking(
      *context.paint_invalidation_container, result);

  result.Move(object.ScrollAdjustmentForPaintInvalidation(
      *context.paint_invalidation_container));

  return result;
}

void PaintInvalidatorContext::MapLocalRectToVisualRectInBacking(
    const LayoutObject& object,
    LayoutRect& rect) const {
  rect = PaintInvalidator::MapLocalRectToVisualRectInBacking<LayoutRect,
                                                             LayoutPoint>(
      object, rect, *this);
}

const PaintInvalidatorContext*
PaintInvalidatorContext::ParentContextAccessor::ParentContext() const {
  return tree_walk_ ? &tree_walk_->ContextAt(parent_context_index_)
                           .paint_invalidator_context
                    : nullptr;
}

LayoutRect PaintInvalidator::ComputeVisualRectInBacking(
    const LayoutObject& object,
    const PaintInvalidatorContext& context) {
  if (object.IsSVGChild()) {
    FloatRect local_rect = SVGLayoutSupport::LocalVisualRect(object);
    return MapLocalRectToVisualRectInBacking<FloatRect, FloatPoint>(
        object, local_rect, context);
  }
  LayoutRect local_rect = object.LocalVisualRect();
  return MapLocalRectToVisualRectInBacking<LayoutRect, LayoutPoint>(
      object, local_rect, context);
}

LayoutRect PaintInvalidator::ComputeVisualRectInBacking(
    const NGPaintFragment& fragment,
    const LayoutObject& object,
    const PaintInvalidatorContext& context) {
  const NGPhysicalFragment& physical_fragment = fragment.PhysicalFragment();
  LayoutRect local_rect =
      physical_fragment.VisualRectWithContents().ToLayoutRect();
  bool disable_flip = true;
  LayoutRect backing_rect =
      MapLocalRectToVisualRectInBacking<LayoutRect, LayoutPoint>(
          object, local_rect, context, disable_flip);
  if (!object.IsBox())
    backing_rect.Move(fragment.InlineOffsetToContainerBox().ToLayoutSize());
  return backing_rect;
}

LayoutPoint PaintInvalidator::ComputeLocationInBacking(
    const LayoutObject& object,
    const PaintInvalidatorContext& context) {
  // In SPv2, locationInBacking is in the space of their local transform node.
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled())
    return object.FirstFragment().PaintOffset();

  LayoutPoint point;
  if (object != context.paint_invalidation_container) {
    point.MoveBy(context.fragment_data->PaintOffset());

    const auto* container_transform =
        context.paint_invalidation_container->FirstFragment()
            .ContentsProperties()
            .Transform();
    if (context.tree_builder_context_->current.transform !=
        container_transform) {
      FloatRect rect = FloatRect(FloatPoint(point), FloatSize());
      GeometryMapper::SourceToDestinationRect(
          context.tree_builder_context_->current.transform, container_transform,
          rect);
      point = LayoutPoint(rect.Location());
    }

    // Convert the result to the paint invalidation container's contents space.
    // If the paint invalidation container has a transform node associated
    // with it (due to scroll or transform), then its PaintOffset
    // must be zero or equal to its subpixel accumulation, since in all
    // such cases we allocate a paint offset translation transform.
    point.MoveBy(
        -context.paint_invalidation_container->FirstFragment().PaintOffset());
  }

  if (context.paint_invalidation_container->Layer()->GroupedMapping()) {
    FloatPoint float_point(point);
    PaintLayer::MapPointInPaintInvalidationContainerToBacking(
        *context.paint_invalidation_container, float_point);
    point = LayoutPoint(float_point);
  }

  point.Move(object.ScrollAdjustmentForPaintInvalidation(
      *context.paint_invalidation_container));

  return point;
}

void PaintInvalidator::UpdatePaintingLayer(const LayoutObject& object,
                                           PaintInvalidatorContext& context) {
  if (object.HasLayer() &&
      ToLayoutBoxModelObject(object).HasSelfPaintingLayer()) {
    context.painting_layer = ToLayoutBoxModelObject(object).Layer();
  } else if (object.IsColumnSpanAll() ||
             object.IsFloatingWithNonContainingBlockParent()) {
    // See LayoutObject::paintingLayer() for the special-cases of floating under
    // inline and multicolumn.
    context.painting_layer = object.PaintingLayer();
  }

  if (object.IsLayoutBlockFlow() && ToLayoutBlockFlow(object).ContainsFloats())
    context.painting_layer->SetNeedsPaintPhaseFloat();

  // Table collapsed borders are painted in PaintPhaseDescendantBlockBackgrounds
  // on the table's layer.
  if (object.IsTable() && ToLayoutTable(object).HasCollapsedBorders())
    context.painting_layer->SetNeedsPaintPhaseDescendantBlockBackgrounds();

  // The following flags are for descendants of the layer object only.
  if (object == context.painting_layer->GetLayoutObject())
    return;

  if (object.IsTableSection()) {
    const auto& section = ToLayoutTableSection(object);
    if (section.Table()->HasColElements())
      context.painting_layer->SetNeedsPaintPhaseDescendantBlockBackgrounds();
  }

  if (object.StyleRef().HasOutline())
    context.painting_layer->SetNeedsPaintPhaseDescendantOutlines();

  if (object.HasBoxDecorationBackground()
      // We also paint overflow controls in background phase.
      || (object.HasOverflowClip() &&
          ToLayoutBox(object).GetScrollableArea()->HasOverflowControls())) {
    context.painting_layer->SetNeedsPaintPhaseDescendantBlockBackgrounds();
  }
}

void PaintInvalidator::UpdatePaintInvalidationContainer(
    const LayoutObject& object,
    PaintInvalidatorContext& context) {
  if (object.IsPaintInvalidationContainer()) {
    context.paint_invalidation_container = ToLayoutBoxModelObject(&object);
    if (object.StyleRef().IsStackingContext())
      context.paint_invalidation_container_for_stacked_contents =
          ToLayoutBoxModelObject(&object);
  } else if (object.IsLayoutView()) {
    // paintInvalidationContainerForStackedContents is only for stacked
    // descendants in its own frame, because it doesn't establish stacking
    // context for stacked contents in sub-frames.
    // Contents stacked in the root stacking context in this frame should use
    // this frame's paintInvalidationContainer.
    context.paint_invalidation_container_for_stacked_contents =
        context.paint_invalidation_container;
  } else if (object.IsFloatingWithNonContainingBlockParent() ||
             object.IsColumnSpanAll()) {
    // In these cases, the object may belong to an ancestor of the current
    // paint invalidation container, in paint order.
    context.paint_invalidation_container =
        &object.ContainerForPaintInvalidation();
  } else if (object.StyleRef().IsStacked() &&
             // This is to exclude some objects (e.g. LayoutText) inheriting
             // stacked style from parent but aren't actually stacked.
             object.HasLayer() &&
             context.paint_invalidation_container !=
                 context.paint_invalidation_container_for_stacked_contents) {
    // The current object is stacked, so we should use
    // m_paintInvalidationContainerForStackedContents as its paint invalidation
    // container on which the current object is painted.
    context.paint_invalidation_container =
        context.paint_invalidation_container_for_stacked_contents;
    if (context.subtree_flags &
        PaintInvalidatorContext::kSubtreeFullInvalidationForStackedContents) {
      context.subtree_flags |=
          PaintInvalidatorContext::kSubtreeFullInvalidation;
    }
  }

  if (object == context.paint_invalidation_container) {
    // When we hit a new paint invalidation container, we don't need to
    // continue forcing a check for paint invalidation, since we're
    // descending into a different invalidation container. (For instance if
    // our parents were moved, the entire container will just move.)
    if (object != context.paint_invalidation_container_for_stacked_contents) {
      // However, we need to keep kSubtreeVisualRectUpdate and
      // kSubtreeFullInvalidationForStackedContents flags if the current
      // object isn't the paint invalidation container of stacked contents.
      context.subtree_flags &=
          (PaintInvalidatorContext::kSubtreeVisualRectUpdate |
           PaintInvalidatorContext::kSubtreeFullInvalidationForStackedContents);
    } else {
      context.subtree_flags = 0;
    }
  }

  DCHECK(context.paint_invalidation_container ==
         object.ContainerForPaintInvalidation());
  DCHECK(context.painting_layer == object.PaintingLayer());
}

void PaintInvalidator::UpdateVisualRect(const LayoutObject& object,
                                        FragmentData& fragment_data,
                                        PaintInvalidatorContext& context) {
  if (!context.NeedsVisualRectUpdate(object))
    return;

  DCHECK(context.tree_builder_context_);
  DCHECK(context.tree_builder_context_->current.paint_offset ==
         fragment_data.PaintOffset());

  LayoutRect new_visual_rect = ComputeVisualRectInBacking(object, context);
  LayoutPoint new_location;
  if (object.IsBoxModelObject()) {
    new_location = ComputeLocationInBacking(object, context);
    // Location of empty visual rect doesn't affect paint invalidation. Set it
    // to newLocation to avoid saving the previous location separately in
    // ObjectPaintInvalidator.
    if (new_visual_rect.IsEmpty())
      new_visual_rect.SetLocation(new_location);
  } else {
    // Use visual rect location for non-LayoutBoxModelObject because it suffices
    // to check whether a visual rect changes for layout caused invalidation.
    new_location = new_visual_rect.Location();
  }

  fragment_data.SetVisualRect(new_visual_rect);
  fragment_data.SetLocationInBacking(new_location);

  // For LayoutNG, update NGPaintFragments.
  if (!RuntimeEnabledFeatures::LayoutNGEnabled())
    return;

  // TODO(kojii): multi-col needs additional logic. What's needed is to be
  // figured out.
  if (object.IsLayoutNGMixin()) {
    if (NGPaintFragment* fragment = ToLayoutBlockFlow(object).PaintFragment())
      fragment->SetVisualRect(new_visual_rect);

    // Also check IsInline below. Inline block is LayoutBlockFlow but is also in
    // an inline formatting context.
  }

  if (object.IsInline()) {
    // An inline LayoutObject can produce multiple NGPaintFragment. Compute
    // VisualRect for each fragment from |new_visual_rect|.
    auto fragments = NGPaintFragment::InlineFragmentsFor(&object);
    if (fragments.IsInLayoutNGInlineFormattingContext()) {
      for (NGPaintFragment* fragment : fragments) {
        LayoutRect fragment_visual_rect =
            ComputeVisualRectInBacking(*fragment, object, context);
        fragment->SetVisualRect(fragment_visual_rect);
      }
    }
  }
}

void PaintInvalidator::InvalidatePaint(
    LocalFrameView& frame_view,
    const PaintPropertyTreeBuilderContext* tree_builder_context,

    PaintInvalidatorContext& context) {
  LayoutView* layout_view = frame_view.GetLayoutView();
  CHECK(layout_view);

  context.paint_invalidation_container =
      context.paint_invalidation_container_for_stacked_contents =
          &layout_view->ContainerForPaintInvalidation();
  context.painting_layer = layout_view->Layer();
  context.fragment_data = &layout_view->FirstFragment();
  if (tree_builder_context) {
    context.tree_builder_context_ = &tree_builder_context->fragments[0];
#if DCHECK_IS_ON()
    context.tree_builder_context_actually_needed_ =
        tree_builder_context->is_actually_needed;
#endif
  }
}

static void InvalidateChromeClient(
    const LayoutBoxModelObject& paint_invalidation_container) {
  if (paint_invalidation_container.GetDocument().Printing() &&
      !RuntimeEnabledFeatures::PrintBrowserEnabled())
    return;

  DCHECK(paint_invalidation_container.IsLayoutView());
  DCHECK(!paint_invalidation_container.IsPaintInvalidationContainer());

  auto* frame_view = paint_invalidation_container.GetFrameView();
  DCHECK(!frame_view->GetFrame().OwnerLayoutObject());
  if (auto* client = frame_view->GetChromeClient()) {
    client->InvalidateRect(
        frame_view->ContentsToFrame(frame_view->VisibleContentRect()));
  }
}

void PaintInvalidator::UpdateEmptyVisualRectFlag(
    const LayoutObject& object,
    PaintInvalidatorContext& context) {
  bool is_paint_invalidation_container =
      object == context.paint_invalidation_container;

  // Content under transforms needs to invalidate, even if visual
  // rects before and after update were the same. This is because
  // we don't know whether this transform will end up composited in
  // SPv2, so such transforms are painted even if not visible
  // due to ancestor clips. This does not apply in SPv1 mode when
  // crossing paint invalidation container boundaries.
  if (is_paint_invalidation_container) {
    // Remove the flag when crossing paint invalidation container boundaries.
    context.subtree_flags &=
        ~PaintInvalidatorContext::kInvalidateEmptyVisualRect;
  } else if (object.StyleRef().HasTransform()) {
    context.subtree_flags |=
        PaintInvalidatorContext::kInvalidateEmptyVisualRect;
  }
}

void PaintInvalidator::InvalidatePaint(
    const LayoutObject& object,
    const PaintPropertyTreeBuilderContext* tree_builder_context,
    PaintInvalidatorContext& context) {
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("blink.invalidation"),
               "PaintInvalidator::InvalidatePaint()", "object",
               object.DebugName().Ascii());

  if (object.IsSVGHiddenContainer()) {
    context.subtree_flags |= PaintInvalidatorContext::kSubtreeNoInvalidation;
  }
  if (context.subtree_flags & PaintInvalidatorContext::kSubtreeNoInvalidation)
    return;

  object.GetMutableForPainting().EnsureIsReadyForPaintInvalidation();

  UpdatePaintingLayer(object, context);

  // TODO(chrishtr): refactor to remove these slow paths by expanding their
  // LocalVisualRect to include repeated locations.
  if (object.IsTableSection()) {
    const auto& section = ToLayoutTableSection(object);
    if (section.IsRepeatingHeaderGroup() || section.IsRepeatingFooterGroup())
      context.subtree_flags |= PaintInvalidatorContext::kSubtreeSlowPathRect;
  }
  if (object.IsFixedPositionObjectInPagedMedia())
    context.subtree_flags |= PaintInvalidatorContext::kSubtreeSlowPathRect;

  UpdatePaintInvalidationContainer(object, context);
  UpdateEmptyVisualRectFlag(object, context);

  if (!object.ShouldCheckForPaintInvalidation() && !context.subtree_flags)
    return;

  unsigned tree_builder_index = 0;

  for (auto *fragment_data = &object.GetMutableForPainting().FirstFragment();
       fragment_data;
       fragment_data = fragment_data->NextFragment(), tree_builder_index++) {
    context.old_visual_rect = fragment_data->VisualRect();
    context.old_location = fragment_data->LocationInBacking();
    context.fragment_data = fragment_data;

    DCHECK(!tree_builder_context ||
           tree_builder_index < tree_builder_context->fragments.size());

    {
#if DCHECK_IS_ON()
      bool is_actually_needed =
          tree_builder_context && tree_builder_context->is_actually_needed;
      FindObjectVisualRectNeedingUpdateScope finder(
          object, *fragment_data, context, is_actually_needed);

      context.tree_builder_context_actually_needed_ = is_actually_needed;
#endif
      if (tree_builder_context) {
        context.tree_builder_context_ =
            &tree_builder_context->fragments[tree_builder_index];
      } else {
        context.tree_builder_context_ = nullptr;
      }

      UpdateVisualRect(object, *fragment_data, context);
    }

    PaintInvalidationReason reason = object.InvalidatePaint(context);
    switch (reason) {
      case PaintInvalidationReason::kDelayedFull:
        pending_delayed_paint_invalidations_.push_back(&object);
        break;
      case PaintInvalidationReason::kSubtree:
        context.subtree_flags |=
            (PaintInvalidatorContext::kSubtreeFullInvalidation |
             PaintInvalidatorContext::
                 kSubtreeFullInvalidationForStackedContents);
        break;
      case PaintInvalidationReason::kSVGResource:
        context.subtree_flags |=
            PaintInvalidatorContext::kSubtreeSVGResourceChange;
        break;
      default:
        break;
    }

    if (context.old_location != fragment_data->LocationInBacking() &&
        !context.painting_layer->SubtreeIsInvisible()) {
      context.subtree_flags |=
          PaintInvalidatorContext::kSubtreeInvalidationChecking;
    }
  }

  if (object.MayNeedPaintInvalidationSubtree()) {
    context.subtree_flags |=
        PaintInvalidatorContext::kSubtreeInvalidationChecking;
  }

  if (context.subtree_flags && context.NeedsVisualRectUpdate(object)) {
    // If any subtree flag is set, we also need to pass needsVisualRectUpdate
    // requirement to the subtree.
    context.subtree_flags |= PaintInvalidatorContext::kSubtreeVisualRectUpdate;
  }

  if (context.NeedsVisualRectUpdate(object) &&
      object.ContainsInlineWithOutlineAndContinuation()) {
    // Force subtree visual rect update and invalidation checking to ensure
    // invalidation of focus rings when continuation's geometry changes.
    context.subtree_flags |=
        PaintInvalidatorContext::kSubtreeVisualRectUpdate |
        PaintInvalidatorContext::kSubtreeInvalidationChecking;
  }

  // The object is under a frame for WebViewPlugin, SVG images etc. Need to
  // inform the chrome client of the invalidation so that the client will
  // initiate painting of the contents. For SPv1 this is done by
  // ObjectPaintInvalidator::InvalidatePaintUsingContainer().
  // TODO(wangxianzhu): Do we need this for SPv2?
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      !RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
      !context.paint_invalidation_container->IsPaintInvalidationContainer() &&
      object.GetPaintInvalidationReason() != PaintInvalidationReason::kNone)
    InvalidateChromeClient(*context.paint_invalidation_container);
}

void PaintInvalidator::ProcessPendingDelayedPaintInvalidations() {
  for (auto* target : pending_delayed_paint_invalidations_) {
    target->GetMutableForPainting()
        .SetShouldDoFullPaintInvalidationWithoutGeometryChange(
            PaintInvalidationReason::kDelayedFull);
  }
}

}  // namespace blink
