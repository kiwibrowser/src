// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/pre_paint_tree_walk.h"

#include "base/auto_reset.h"
#include "third_party/blink/renderer/core/dom/document_lifecycle.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_multi_column_spanner_placeholder.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/compositing_layer_property_updater.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_property_tree_printer.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"

namespace blink {

void PrePaintTreeWalk::WalkTree(LocalFrameView& root_frame_view) {
  if (root_frame_view.ShouldThrottleRendering()) {
    // Skip the throttled frame. Will update it when it becomes unthrottled.
    return;
  }

  DCHECK(root_frame_view.GetFrame().GetDocument()->Lifecycle().GetState() ==
         DocumentLifecycle::kInPrePaint);

  // Reserve 50 elements for a really deep DOM. If the nesting is deeper than
  // this, then the vector will reallocate, but it shouldn't be a big deal. This
  // is also temporary within this function.
  DCHECK_EQ(context_storage_.size(), 0u);
  context_storage_.ReserveCapacity(50);
  context_storage_.emplace_back();

  // GeometryMapper depends on paint properties.
  bool needs_tree_builder_context_update =
      NeedsTreeBuilderContextUpdate(root_frame_view, context_storage_.back());
  if (needs_tree_builder_context_update)
    GeometryMapper::ClearCache();

  Walk(root_frame_view);
  paint_invalidator_.ProcessPendingDelayedPaintInvalidations();
  context_storage_.pop_back();

#if DCHECK_IS_ON()
  if (!needs_tree_builder_context_update)
    return;
  if (VLOG_IS_ON(2) && root_frame_view.GetLayoutView()) {
    LOG(ERROR) << "PrePaintTreeWalk::Walk(root_frame_view=" << &root_frame_view
               << ")\nPaintLayer tree:";
    showLayerTree(root_frame_view.GetLayoutView()->Layer());
  }
  if (VLOG_IS_ON(1))
    showAllPropertyTrees(root_frame_view);
#endif
}

void PrePaintTreeWalk::Walk(LocalFrameView& frame_view) {
  if (frame_view.ShouldThrottleRendering()) {
    // Skip the throttled frame. Will update it when it becomes unthrottled.
    return;
  }

  // We need to be careful not to have a reference to the parent context, since
  // this reference will be to the context_storage_ memory which may be
  // reallocated during this function call.
  size_t parent_context_index = context_storage_.size() - 1;
  auto parent_context = [this,
                         parent_context_index]() -> PrePaintTreeWalkContext& {
    return context_storage_[parent_context_index];
  };

  bool needs_tree_builder_context_update =
      NeedsTreeBuilderContextUpdate(frame_view, parent_context());

  // Note that because we're emplacing an object constructed from
  // parent_context() (which is a reference to the vector itself), it's
  // important to first ensure that there's sufficient capacity in the vector.
  // Otherwise, it may reallocate causing parent_context() to point to invalid
  // memory.
  ResizeContextStorageIfNeeded();
  context_storage_.emplace_back(parent_context(),
                                PaintInvalidatorContext::ParentContextAccessor(
                                    this, parent_context_index),
                                needs_tree_builder_context_update);
  auto context = [this]() -> PrePaintTreeWalkContext& {
    return context_storage_.back();
  };

  // ancestorOverflowLayer does not cross frame boundaries.
  context().ancestor_overflow_paint_layer = nullptr;
  if (context().tree_builder_context) {
    PaintPropertyTreeBuilder::SetupContextForFrame(
        frame_view, *context().tree_builder_context);
  }
  paint_invalidator_.InvalidatePaint(
      frame_view, base::OptionalOrNullptr(context().tree_builder_context),
      context().paint_invalidator_context);
  if (context().tree_builder_context) {
    context().tree_builder_context->supports_composited_raster_invalidation =
        frame_view.GetFrame().GetSettings()->GetAcceleratedCompositingEnabled();
  }

  if (LayoutView* view = frame_view.GetLayoutView()) {
#ifndef NDEBUG
    if (VLOG_IS_ON(3) && needs_tree_builder_context_update) {
      LOG(ERROR) << "PrePaintTreeWalk::Walk(frame_view=" << &frame_view
                 << ")\nLayout tree:";
      showLayoutTree(view);
    }
#endif

    Walk(*view);
#if DCHECK_IS_ON()
    view->AssertSubtreeClearedPaintInvalidationFlags();
#endif
  }

  frame_view.ClearNeedsPaintPropertyUpdate();
  if (RuntimeEnabledFeatures::JankTrackingEnabled())
    frame_view.GetJankTracker().NotifyPrePaintFinished();
  context_storage_.pop_back();
}

void PrePaintTreeWalk::UpdateAuxiliaryObjectProperties(
    const LayoutObject& object,
    PrePaintTreeWalk::PrePaintTreeWalkContext& context) {
  if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  if (!object.HasLayer())
    return;

  PaintLayer* paint_layer = ToLayoutBoxModelObject(object).Layer();
  paint_layer->UpdateAncestorOverflowLayer(
      context.ancestor_overflow_paint_layer);

  if (object.StyleRef().HasStickyConstrainedPosition()) {
    paint_layer->GetLayoutObject().UpdateStickyPositionConstraints();

    // Sticky position constraints and ancestor overflow scroller affect the
    // sticky layer position, so we need to update it again here.
    // TODO(flackr): This should be refactored in the future to be clearer (i.e.
    // update layer position and ancestor inputs updates in the same walk).
    paint_layer->UpdateLayerPosition();
  }
  if (paint_layer->IsRootLayer() || object.HasOverflowClip())
    context.ancestor_overflow_paint_layer = paint_layer;
}

void PrePaintTreeWalk::InvalidatePaintLayerOptimizationsIfNeeded(
    const LayoutObject& object,
    PrePaintTreeWalkContext& context) {
  if (!object.HasLayer())
    return;

  PaintLayer& paint_layer = *ToLayoutBoxModelObject(object).Layer();

  if (!context.tree_builder_context->clip_changed)
    return;

  paint_layer.SetNeedsRepaint();
  paint_layer.SetPreviousPaintPhaseDescendantOutlinesEmpty(false);
  paint_layer.SetPreviousPaintPhaseFloatEmpty(false);
  paint_layer.SetPreviousPaintPhaseDescendantBlockBackgroundsEmpty(false);
  context.paint_invalidator_context.subtree_flags |=
      PaintInvalidatorContext::kSubtreeVisualRectUpdate;
}

bool PrePaintTreeWalk::NeedsTreeBuilderContextUpdate(
    const LocalFrameView& frame_view,
    const PrePaintTreeWalkContext& context) {
  return frame_view.NeedsPaintPropertyUpdate() ||
         (frame_view.GetLayoutView() &&
          NeedsTreeBuilderContextUpdate(*frame_view.GetLayoutView(), context));
}

bool PrePaintTreeWalk::NeedsTreeBuilderContextUpdate(
    const LayoutObject& object,
    const PrePaintTreeWalkContext& parent_context) {
  if (parent_context.tree_builder_context &&
      parent_context.tree_builder_context->force_subtree_update) {
    return true;
  }
  // The following CHECKs are for debugging crbug.com/816810.
  if (object.NeedsPaintPropertyUpdate()) {
    CHECK(parent_context.tree_builder_context) << "NeedsPaintPropertyUpdate";
    return true;
  }
  if (object.DescendantNeedsPaintPropertyUpdate()) {
    CHECK(parent_context.tree_builder_context)
        << "DescendantNeedsPaintPropertyUpdate";
    return true;
  }
  if (parent_context.paint_invalidator_context.NeedsVisualRectUpdate(object)) {
    // If the object needs visual rect update, we should update tree
    // builder context which is needed by visual rect update.
    if (object.NeedsPaintOffsetAndVisualRectUpdate()) {
      CHECK(parent_context.tree_builder_context)
          << "NeedsPaintOffsetAndVisualRectUpdate";
    } else {
      CHECK(parent_context.tree_builder_context) << "kSubtreeVisualRectUpdate";
    }
    return true;
  }
  return false;
}

void PrePaintTreeWalk::WalkInternal(const LayoutObject& object,
                                    PrePaintTreeWalkContext& context) {
  PaintInvalidatorContext& paint_invalidator_context =
      context.paint_invalidator_context;

  // This must happen before updatePropertiesForSelf, because the latter reads
  // some of the state computed here.
  UpdateAuxiliaryObjectProperties(object, context);

  base::Optional<PaintPropertyTreeBuilder> property_tree_builder;
  bool property_changed = false;
  if (context.tree_builder_context) {
    property_tree_builder.emplace(object, *context.tree_builder_context);
    property_changed = property_tree_builder->UpdateForSelf();

    if (context.tree_builder_context->clip_changed) {
      paint_invalidator_context.subtree_flags |=
          PaintInvalidatorContext::kSubtreeVisualRectUpdate;
    }

    if (property_changed &&
        RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        !context.tree_builder_context
             ->supports_composited_raster_invalidation) {
      paint_invalidator_context.subtree_flags |=
          PaintInvalidatorContext::kSubtreeFullInvalidation;
    }
  }

  paint_invalidator_.InvalidatePaint(
      object, base::OptionalOrNullptr(context.tree_builder_context),
      paint_invalidator_context);

  if (context.tree_builder_context) {
    property_changed |= property_tree_builder->UpdateForChildren();
    InvalidatePaintLayerOptimizationsIfNeeded(object, context);

    if (property_changed &&
        RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
        const auto* paint_invalidation_layer =
            paint_invalidator_context.paint_invalidation_container->Layer();
        if (!paint_invalidation_layer->NeedsRepaint()) {
          auto* mapping = paint_invalidation_layer->GetCompositedLayerMapping();
          if (!mapping)
            mapping = paint_invalidation_layer->GroupedMapping();
          if (mapping)
            mapping->SetNeedsCheckRasterInvalidation();
        }
      } else if (!context.tree_builder_context
                      ->supports_composited_raster_invalidation) {
        paint_invalidator_context.subtree_flags |=
            PaintInvalidatorContext::kSubtreeFullInvalidation;
      }
    }
  }

  CompositingLayerPropertyUpdater::Update(object);

  if (RuntimeEnabledFeatures::JankTrackingEnabled()) {
    object.GetFrameView()->GetJankTracker().NotifyObjectPrePaint(
        object, paint_invalidator_context.old_visual_rect,
        *paint_invalidator_context.painting_layer);
  }
}

void PrePaintTreeWalk::Walk(const LayoutObject& object) {
  // We need to be careful not to have a reference to the parent context, since
  // this reference will be to the context_storage_ memory which may be
  // reallocated during this function call.
  size_t parent_context_index = context_storage_.size() - 1;
  auto parent_context = [this,
                         parent_context_index]() -> PrePaintTreeWalkContext& {
    return context_storage_[parent_context_index];
  };

  bool needs_tree_builder_context_update =
      NeedsTreeBuilderContextUpdate(object, parent_context());
  // Early out from the tree walk if possible.
  if (!needs_tree_builder_context_update &&
      !object.ShouldCheckForPaintInvalidation()) {
    return;
  }

  // Note that because we're emplacing an object constructed from
  // parent_context() (which is a reference to the vector itself), it's
  // important to first ensure that there's sufficient capacity in the vector.
  // Otherwise, it may reallocate causing parent_context() to point to invalid
  // memory.
  ResizeContextStorageIfNeeded();
  context_storage_.emplace_back(parent_context(),
                                PaintInvalidatorContext::ParentContextAccessor(
                                    this, parent_context_index),
                                needs_tree_builder_context_update);
  auto context = [this]() -> PrePaintTreeWalkContext& {
    return context_storage_.back();
  };

  // Ignore clip changes from ancestor across transform boundaries.
  if (context().tree_builder_context && object.StyleRef().HasTransform())
    context().tree_builder_context->clip_changed = false;

  WalkInternal(object, context());

  for (const LayoutObject* child = object.SlowFirstChild(); child;
       child = child->NextSibling()) {
    if (child->IsLayoutMultiColumnSpannerPlaceholder()) {
      child->GetMutableForPainting().ClearPaintFlags();
      continue;
    }
    Walk(*child);
  }

  if (object.IsLayoutEmbeddedContent()) {
    const LayoutEmbeddedContent& layout_embedded_content =
        ToLayoutEmbeddedContent(object);
    FrameView* frame_view = layout_embedded_content.ChildFrameView();
    if (frame_view && frame_view->IsLocalFrameView()) {
      LocalFrameView* local_frame_view = ToLocalFrameView(frame_view);
      if (context().tree_builder_context) {
        context().tree_builder_context->fragments[0].current.paint_offset +=
            layout_embedded_content.ReplacedContentRect().Location() -
            local_frame_view->FrameRect().Location();
        context()
            .tree_builder_context->fragments[0]
            .current.paint_offset = RoundedIntPoint(
            context().tree_builder_context->fragments[0].current.paint_offset);
      }
      Walk(*local_frame_view);
    }
    // TODO(pdr): Investigate RemoteFrameView (crbug.com/579281).
  }

  // Because current |PrePaintTreeWalk| walks LayoutObject tree, NGPaintFragment
  // that are not mapped to LayoutObject are not updated. Ensure they are
  // updated after all descendants were updated.
  if (RuntimeEnabledFeatures::LayoutNGEnabled() && object.IsLayoutNGMixin()) {
    if (NGPaintFragment* fragment = ToLayoutBlockFlow(object).PaintFragment())
      fragment->UpdateVisualRectForNonLayoutObjectChildren();
  }

  object.GetMutableForPainting().ClearPaintFlags();
  context_storage_.pop_back();
}

void PrePaintTreeWalk::ResizeContextStorageIfNeeded() {
  if (UNLIKELY(context_storage_.size() == context_storage_.capacity())) {
    DCHECK_GT(context_storage_.size(), 0u);
    context_storage_.ReserveCapacity(context_storage_.size() * 2);
  }
}

}  // namespace blink
