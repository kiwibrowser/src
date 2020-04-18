/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2004 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INFO_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INFO_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
// TODO(jchaffraix): Once we unify PaintBehavior and PaintLayerFlags, we should
// move PaintLayerFlags to PaintPhase and rename it. Thus removing the need for
// this #include
// "third_party/blink/renderer/core/paint/paint_layer_painting_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painting_info.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/image.h"
#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

#include <limits>

namespace blink {

class LayoutBoxModelObject;

struct CORE_EXPORT PaintInfo {
  USING_FAST_MALLOC(PaintInfo);

 public:
  PaintInfo(GraphicsContext& context,
            const IntRect& cull_rect,
            PaintPhase phase,
            GlobalPaintFlags global_paint_flags,
            PaintLayerFlags paint_flags,
            const LayoutBoxModelObject* paint_container = nullptr,
            LayoutUnit fragment_logical_top_in_flow_thread = LayoutUnit(),
            bool suppress_painting_descendants = false)
      : context(context),
        phase(phase),
        cull_rect_(cull_rect),
        paint_container_(paint_container),
        fragment_logical_top_in_flow_thread_(
            fragment_logical_top_in_flow_thread),
        paint_flags_(paint_flags),
        global_paint_flags_(global_paint_flags),
        suppress_painting_descendants_(suppress_painting_descendants) {}

  PaintInfo(GraphicsContext& new_context,
            const PaintInfo& copy_other_fields_from)
      : context(new_context),
        phase(copy_other_fields_from.phase),
        cull_rect_(copy_other_fields_from.cull_rect_),
        paint_container_(copy_other_fields_from.paint_container_),
        fragment_logical_top_in_flow_thread_(
            copy_other_fields_from.fragment_logical_top_in_flow_thread_),
        paint_flags_(copy_other_fields_from.paint_flags_),
        global_paint_flags_(copy_other_fields_from.global_paint_flags_),
        suppress_painting_descendants_(
            copy_other_fields_from.suppress_painting_descendants_) {}

  // Creates a PaintInfo for painting descendants. See comments about the paint
  // phases in PaintPhase.h for details.
  PaintInfo ForDescendants() const {
    PaintInfo result(*this);
    if (phase == PaintPhase::kDescendantOutlinesOnly)
      result.phase = PaintPhase::kOutline;
    else if (phase == PaintPhase::kDescendantBlockBackgroundsOnly)
      result.phase = PaintPhase::kBlockBackground;
    return result;
  }

  bool IsRenderingClipPathAsMaskImage() const {
    return paint_flags_ & kPaintLayerPaintingRenderingClipPathAsMask;
  }
  bool IsRenderingResourceSubtree() const {
    return paint_flags_ & kPaintLayerPaintingRenderingResourceSubtree;
  }

  bool SkipRootBackground() const {
    return paint_flags_ & kPaintLayerPaintingSkipRootBackground;
  }

  bool IsPrinting() const { return global_paint_flags_ & kGlobalPaintPrinting; }

  bool SuppressPaintingDescendants() const {
    return suppress_painting_descendants_;
  }

  DisplayItem::Type DisplayItemTypeForClipping() const {
    return DisplayItem::PaintPhaseToClipBoxType(phase);
  }

  const LayoutBoxModelObject* PaintContainer() const {
    return paint_container_;
  }

  GlobalPaintFlags GetGlobalPaintFlags() const { return global_paint_flags_; }

  PaintLayerFlags PaintFlags() const { return paint_flags_; }

  const CullRect& GetCullRect() const { return cull_rect_; }

  void UpdateCullRect(const AffineTransform& local_to_parent_transform) {
    cull_rect_.UpdateCullRect(local_to_parent_transform);
  }

  void UpdateCullRectForScrollingContents(
      const IntRect& overflow_clip_rect,
      const AffineTransform& local_to_parent_transform) {
    cull_rect_.UpdateForScrollingContents(overflow_clip_rect,
                                          local_to_parent_transform);
  }

  // Returns the fragment of the current painting object matching the current
  // layer fragment.
  const FragmentData* FragmentToPaint(const LayoutObject& object) const {
    for (const auto* fragment = &object.FirstFragment(); fragment;
         fragment = fragment->NextFragment()) {
      if (fragment->LogicalTopInFlowThread() ==
          fragment_logical_top_in_flow_thread_)
        return fragment;
    }
    // No fragment of the current painting object matches the layer fragment,
    // which means the object should not paint in this fragment.
    return nullptr;
  }

  // FIXME: Introduce setters/getters at some point. Requires a lot of changes
  // throughout layout/.
  GraphicsContext& context;
  PaintPhase phase;

 private:
  CullRect cull_rect_;

  // The box model object that originates the current painting.
  const LayoutBoxModelObject* paint_container_;

  // The logical top of the current fragment of the self-painting PaintLayer
  // which initiated the current painting, in the containing flow thread.
  LayoutUnit fragment_logical_top_in_flow_thread_;

  const PaintLayerFlags paint_flags_;
  const GlobalPaintFlags global_paint_flags_;
  const bool suppress_painting_descendants_;

  // TODO(chrishtr): temporary while we implement CullRect everywhere.
  friend class SVGPaintContext;
  friend class SVGShapePainter;
};

Image::ImageDecodingMode GetImageDecodingMode(Node*);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_INFO_H_
