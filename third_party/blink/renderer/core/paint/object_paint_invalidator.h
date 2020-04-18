// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_OBJECT_PAINT_INVALIDATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_OBJECT_PAINT_INVALIDATOR_H_

#include "base/auto_reset.h"
#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class DisplayItemClient;
class LayoutBoxModelObject;
class LayoutObject;
class LayoutRect;
struct PaintInvalidatorContext;

class CORE_EXPORT ObjectPaintInvalidator {
  STACK_ALLOCATED();

 public:
  ObjectPaintInvalidator(const LayoutObject& object) : object_(object) {}

  // This calls paintingLayer() which walks up the tree.
  // If possible, use the faster
  // PaintInvalidatorContext.paintingLayer.setNeedsRepaint().
  void SlowSetPaintingLayerNeedsRepaint();

  // TODO(wangxianzhu): Change the call sites to use the faster version if
  // possible.
  void SlowSetPaintingLayerNeedsRepaintAndInvalidateDisplayItemClient(
      const DisplayItemClient& client,
      PaintInvalidationReason reason) {
    SlowSetPaintingLayerNeedsRepaint();
    InvalidateDisplayItemClient(client, reason);
  }

  void InvalidateDisplayItemClientsIncludingNonCompositingDescendants(
      PaintInvalidationReason);

  void InvalidatePaintOfPreviousVisualRect(
      const LayoutBoxModelObject& paint_invalidation_container,
      PaintInvalidationReason);

  // The caller should ensure the painting layer has been setNeedsRepaint before
  // calling this function.
  void InvalidateDisplayItemClient(const DisplayItemClient&,
                                   PaintInvalidationReason);

  // Actually do the paint invalidate of rect r for this object which has been
  // computed in the coordinate space of the GraphicsLayer backing of
  // |paintInvalidationContainer|. Note that this coordinate space is not the
  // same as the local coordinate space of |paintInvalidationContainer| in the
  // presence of layer squashing.
  void InvalidatePaintUsingContainer(
      const LayoutBoxModelObject& paint_invalidation_container,
      const LayoutRect&,
      PaintInvalidationReason);

  void InvalidatePaintIncludingNonCompositingDescendants();
  void InvalidatePaintIncludingNonSelfPaintingLayerDescendants(
      const LayoutBoxModelObject& paint_invalidation_container);

 private:
  void InvalidatePaintIncludingNonSelfPaintingLayerDescendantsInternal(
      const LayoutBoxModelObject& paint_invalidation_container);
  void SetBackingNeedsPaintInvalidationInRect(
      const LayoutBoxModelObject& paint_invalidation_container,
      const LayoutRect&,
      PaintInvalidationReason);

 protected:
  const LayoutObject& object_;
};

class ObjectPaintInvalidatorWithContext : public ObjectPaintInvalidator {
 public:
  ObjectPaintInvalidatorWithContext(const LayoutObject& object,
                                    const PaintInvalidatorContext& context)
      : ObjectPaintInvalidator(object), context_(context) {}

  PaintInvalidationReason InvalidatePaint() {
    return InvalidatePaintWithComputedReason(ComputePaintInvalidationReason());
  }

  PaintInvalidationReason ComputePaintInvalidationReason();
  PaintInvalidationReason InvalidatePaintWithComputedReason(
      PaintInvalidationReason);

  // This function generates a full invalidation, which means invalidating both
  // |oldVisualRect| and |newVisualRect|.  This is the default choice when
  // generating an invalidation, as it is always correct, albeit it may force
  // some extra painting.
  void FullyInvalidatePaint(PaintInvalidationReason,
                            const LayoutRect& old_visual_rect,
                            const LayoutRect& new_visual_rect);

  void InvalidatePaintRectangleWithContext(const LayoutRect&,
                                           PaintInvalidationReason);

 private:
  void InvalidateSelection(PaintInvalidationReason);
  void InvalidatePartialRect(PaintInvalidationReason);
  bool ParentFullyInvalidatedOnSameBacking();

  const PaintInvalidatorContext& context_;
};

// Use this for cases that compositing will change and we have to do immediate
// paint invalidation. TODO(wangxianzhu): Remove this for SPv2 and SPv175 which
// will always invalidate raster after paint.
class DisablePaintInvalidationStateAsserts {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(DisablePaintInvalidationStateAsserts);

 public:
  DisablePaintInvalidationStateAsserts();

 private:
  base::AutoReset<bool> disabler_;
};

}  // namespace blink

#endif
