/*
 * Copyright (C) 2003, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_FRAGMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_FRAGMENT_H_

#include "third_party/blink/renderer/core/paint/clip_rect.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class FragmentData;

// PaintLayerFragment is the representation of a fragment.
// https://drafts.csswg.org/css-break/#fragment
//
// Fragments are a paint/hit-testing only concept in Blink.
// Every box has at least one fragment, even if it is not paginated/fragmented.
// If a box has more than one fragment (i.e. is paginated by an ancestor), the
// fragments are called "paginated fragments". Note that this is a Blink
// vocabulary extension and doesn't come from the specification.
//
// The fragments are collected by calling PaintLayer::collectFragments
// on every box once per paint/hit-testing operation.
struct PaintLayerFragment {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  void SetRects(const LayoutRect& bounds,
                const ClipRect& background,
                const ClipRect& foreground) {
    layer_bounds = bounds;
    background_rect = background;
    foreground_rect = foreground;
  }

  void MoveBy(const LayoutPoint& offset) {
    layer_bounds.MoveBy(offset);
    background_rect.MoveBy(offset);
    foreground_rect.MoveBy(offset);
  }

  void Intersect(const LayoutRect& rect) {
    background_rect.Intersect(rect);
    foreground_rect.Intersect(rect);
  }

  // Set on all fragments.
  //
  // The PaintLayer's size in the associated ClipRectsContext's
  // rootLayer coordinate system. See PaintLayer::m_size for the
  // exact rectangle.
  //
  // TODO(jchaffraix): We should store the rootLayer here to ensure we don't
  // mix coordinate systems by mistake.
  LayoutRect layer_bounds;

  // Set on all fragments.
  //
  // The rectangle used to clip the background.
  //
  // The rectangle is the rectangle-to-paint if no clip applies to the
  // fragment. It is the intersection between the visual overflow rect
  // and any overflow clips or 'clip' properties. It is also intersected with
  // |paginationClip| if it is present.
  //
  // See PaintLayerClipper::calculateRects.
  ClipRect background_rect;

  // Set on all fragments.
  //
  // The rectangle used to clip the content (foreground).
  //
  // The rectangle is the rectangle-to-paint if no clip applies to the
  // fragment. If there is an overflow clip, the rectangle-to-paint is
  // intersected with the border box rect without the scrollbars (content gets
  // clipped at their edge). Also any enclosing 'clip' properties get applied
  // to the intersected rectangle. It is also intersected with
  // |paginationClip| if it is present.
  //
  // See PaintLayerClipper::calculateRects.
  ClipRect foreground_rect;

  // Only set on paginated fragments.
  //
  // The physical translation to apply to shift the layer when
  // painting/hit-testing.
  LayoutPoint pagination_offset;

  const FragmentData* fragment_data = nullptr;
};

typedef Vector<PaintLayerFragment, 1> PaintLayerFragments;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_PAINT_LAYER_FRAGMENT_H_
