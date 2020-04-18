// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATOR_H_

#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/paint/paint_property_tree_builder.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class NGPaintFragment;
class PrePaintTreeWalk;
struct CORE_EXPORT PaintInvalidatorContext {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  class ParentContextAccessor {
   public:
    ParentContextAccessor() = default;
    ParentContextAccessor(PrePaintTreeWalk* tree_walk,
                          size_t parent_context_index)
        : tree_walk_(tree_walk), parent_context_index_(parent_context_index) {}
    const PaintInvalidatorContext* ParentContext() const;

   private:
    PrePaintTreeWalk* tree_walk_ = nullptr;
    size_t parent_context_index_ = 0u;
  };

  PaintInvalidatorContext() = default;

  PaintInvalidatorContext(const ParentContextAccessor& parent_context_accessor)
      : parent_context_accessor_(parent_context_accessor),
        subtree_flags(ParentContext()->subtree_flags),
        paint_invalidation_container(
            ParentContext()->paint_invalidation_container),
        paint_invalidation_container_for_stacked_contents(
            ParentContext()->paint_invalidation_container_for_stacked_contents),
        painting_layer(ParentContext()->painting_layer) {}

  void MapLocalRectToVisualRectInBacking(const LayoutObject&,
                                         LayoutRect&) const;

  bool NeedsVisualRectUpdate(const LayoutObject& object) const {
#if DCHECK_IS_ON()
    if (force_visual_rect_update_for_checking_)
      return true;
#endif
    return object.NeedsPaintOffsetAndVisualRectUpdate() ||
           (subtree_flags & PaintInvalidatorContext::kSubtreeVisualRectUpdate);
  }

  const PaintInvalidatorContext* ParentContext() const {
    return parent_context_accessor_.ParentContext();
  }

 private:
  // Parent context accessor has to be initialized first, so inject the private
  // access block here for that reason.
  ParentContextAccessor parent_context_accessor_;

 public:
  enum SubtreeFlag {
    kSubtreeInvalidationChecking = 1 << 0,
    kSubtreeVisualRectUpdate = 1 << 1,
    kSubtreeFullInvalidation = 1 << 2,
    kSubtreeFullInvalidationForStackedContents = 1 << 3,
    kSubtreeSVGResourceChange = 1 << 4,

    // For repeated objects inside multicolumn.
    kSubtreeSlowPathRect = 1 << 5,

    // When this flag is set, no paint or raster invalidation will be issued
    // for the subtree.
    //
    // Context: some objects in this paint walk, for example SVG resource
    // container subtrees, always paint onto temporary PaintControllers which
    // don't have cache, and don't actually have any raster regions, so they
    // don't need any invalidation. They are used as "painting subroutines"
    // for one or more other locations in SVG.
    kSubtreeNoInvalidation = 1 << 6,

    // Don't skip invalidating because the previous and current visual
    // rects were empty.
    kInvalidateEmptyVisualRect = 1 << 7,
  };
  unsigned subtree_flags = 0;

  // The following fields can be null only before
  // PaintInvalidator::updateContext().

  // The current paint invalidation container for normal flow objects.
  // It is the enclosing composited object.
  const LayoutBoxModelObject* paint_invalidation_container = nullptr;

  // The current paint invalidation container for stacked contents (stacking
  // contexts or positioned objects).  It is the nearest ancestor composited
  // object which establishes a stacking context.  See
  // Source/core/paint/README.md ### PaintInvalidationState for details on how
  // stacked contents' paint invalidation containers differ.
  const LayoutBoxModelObject*
      paint_invalidation_container_for_stacked_contents = nullptr;

  PaintLayer* painting_layer = nullptr;

  // Store the old visual rect in the paint invalidation backing's coordinates.
  // It does *not* account for composited scrolling.
  // See LayoutObject::AdjustVisualRectForCompositedScrolling().
  LayoutRect old_visual_rect;
  // Use LayoutObject::VisualRect() to get the new visual rect.

  // This field and LayoutObject::LocationInBacking() store the old and new
  // origins of the object's local coordinates in the paint invalidation
  // backing's coordinates. They are used to detect layoutObject shifts that
  // force a full invalidation and invalidation check in subtree.
  // The points do *not* account for composited scrolling. See
  // LayoutObject::adjustVisualRectForCompositedScrolling().
  // This field will be removed for SPv175.
  LayoutPoint old_location;
  // Use LayoutObject::LocationInBacking() to get the new location.

  const FragmentData* fragment_data;

 private:
  friend class PaintInvalidator;

  const PaintPropertyTreeBuilderFragmentContext* tree_builder_context_ =
      nullptr;

#if DCHECK_IS_ON()
  bool tree_builder_context_actually_needed_ = false;
  friend class FindVisualRectNeedingUpdateScope;
  friend class FindVisualRectNeedingUpdateScopeBase;
  mutable bool force_visual_rect_update_for_checking_ = false;
#endif
};

class PaintInvalidator {
 public:
  void InvalidatePaint(LocalFrameView&,
                       const PaintPropertyTreeBuilderContext*,
                       PaintInvalidatorContext&);
  void InvalidatePaint(const LayoutObject&,
                       const PaintPropertyTreeBuilderContext*,
                       PaintInvalidatorContext&);

  // Process objects needing paint invalidation on the next frame.
  // See the definition of PaintInvalidationDelayedFull for more details.
  void ProcessPendingDelayedPaintInvalidations();

 private:
  friend struct PaintInvalidatorContext;
  friend class PrePaintTreeWalk;

  template <typename Rect, typename Point>
  static void ExcludeCompositedLayerSubpixelAccumulation(
      const LayoutObject&,
      const PaintInvalidatorContext&,
      Rect&);
  template <typename Rect, typename Point>
  static LayoutRect MapLocalRectToVisualRectInBacking(
      const LayoutObject&,
      const Rect&,
      const PaintInvalidatorContext&,
      bool disable_flip = false);

  ALWAYS_INLINE LayoutRect
  ComputeVisualRectInBacking(const LayoutObject&,
                             const PaintInvalidatorContext&);
  ALWAYS_INLINE LayoutRect
  ComputeVisualRectInBacking(const NGPaintFragment&,
                             const LayoutObject&,
                             const PaintInvalidatorContext&);
  ALWAYS_INLINE LayoutPoint
  ComputeLocationInBacking(const LayoutObject&, const PaintInvalidatorContext&);
  ALWAYS_INLINE void UpdatePaintingLayer(const LayoutObject&,
                                         PaintInvalidatorContext&);
  ALWAYS_INLINE void UpdatePaintInvalidationContainer(const LayoutObject&,
                                                      PaintInvalidatorContext&);
  ALWAYS_INLINE void UpdateEmptyVisualRectFlag(const LayoutObject&,
                                               PaintInvalidatorContext&);
  ALWAYS_INLINE void UpdateVisualRect(const LayoutObject&,
                                      FragmentData&,
                                      PaintInvalidatorContext&);

  Vector<const LayoutObject*> pending_delayed_paint_invalidations_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INVALIDATOR_H_
