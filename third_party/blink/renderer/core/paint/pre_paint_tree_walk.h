// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PRE_PAINT_TREE_WALK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PRE_PAINT_TREE_WALK_H_

#include "third_party/blink/renderer/core/paint/clip_rect.h"
#include "third_party/blink/renderer/core/paint/paint_invalidator.h"
#include "third_party/blink/renderer/core/paint/paint_property_tree_builder.h"

namespace blink {

class LayoutObject;
class LocalFrameView;

// This class walks the whole layout tree, beginning from the root
// LocalFrameView, across frame boundaries. Helper classes are called for each
// tree node to perform actual actions.  It expects to be invoked in InPrePaint
// phase.
class CORE_EXPORT PrePaintTreeWalk {
 public:
  PrePaintTreeWalk() = default;
  void WalkTree(LocalFrameView& root_frame);

 private:
  friend PaintInvalidatorContext::ParentContextAccessor;

  // PrePaintTreewalkContext is large and can lead to stack overflows
  // when recursion is deep so these context objects are allocated on the heap.
  // See: https://crbug.com/698653.
  struct PrePaintTreeWalkContext {
    PrePaintTreeWalkContext() { tree_builder_context.emplace(); }
    PrePaintTreeWalkContext(
        const PrePaintTreeWalkContext& parent_context,
        const PaintInvalidatorContext::ParentContextAccessor&
            parent_context_accessor,
        bool needs_tree_builder_context)
        : paint_invalidator_context(parent_context_accessor),
          ancestor_overflow_paint_layer(
              parent_context.ancestor_overflow_paint_layer) {
      if (needs_tree_builder_context || DCHECK_IS_ON()) {
        DCHECK(parent_context.tree_builder_context);
        tree_builder_context.emplace(*parent_context.tree_builder_context);
      }
#if DCHECK_IS_ON()
      if (needs_tree_builder_context)
        DCHECK(parent_context.tree_builder_context->is_actually_needed);
      tree_builder_context->is_actually_needed = needs_tree_builder_context;
#endif
    }

    base::Optional<PaintPropertyTreeBuilderContext> tree_builder_context;
    PaintInvalidatorContext paint_invalidator_context;

    // The ancestor in the PaintLayer tree which has overflow clip, or
    // is the root layer. Note that it is tree ancestor, not containing
    // block or stacking ancestor.
    PaintLayer* ancestor_overflow_paint_layer = nullptr;
  };

  const PrePaintTreeWalkContext& ContextAt(size_t index) {
    DCHECK_LT(index, context_storage_.size());
    return context_storage_[index];
  }

  void Walk(LocalFrameView&);

  // This is to minimize stack frame usage during recursion. Modern compilers
  // (MSVC in particular) can inline across compilation units, resulting in
  // very big stack frames. Splitting the heavy lifting to a separate function
  // makes sure the stack frame is freed prior to making a recursive call.
  // See https://crbug.com/781301 .
  NOINLINE void WalkInternal(const LayoutObject&, PrePaintTreeWalkContext&);
  void Walk(const LayoutObject&);

  // Invalidates paint-layer painting optimizations, such as subsequence caching
  // and empty paint phase optimizations if clips from the context have changed.
  void InvalidatePaintLayerOptimizationsIfNeeded(const LayoutObject&,
                                                 PrePaintTreeWalkContext&);

  bool NeedsTreeBuilderContextUpdate(const LocalFrameView&,
                                     const PrePaintTreeWalkContext&);
  bool NeedsTreeBuilderContextUpdate(const LayoutObject&,
                                     const PrePaintTreeWalkContext&);
  void UpdateAuxiliaryObjectProperties(const LayoutObject&,
                                       PrePaintTreeWalkContext&);

  void ResizeContextStorageIfNeeded();

  PaintInvalidator paint_invalidator_;
  Vector<PrePaintTreeWalkContext> context_storage_;

  FRIEND_TEST_ALL_PREFIXES(PrePaintTreeWalkTest, ClipRects);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PRE_PAINT_TREE_WALK_H_
