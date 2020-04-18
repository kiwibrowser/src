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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_RECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_RECT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/float_clip_rect.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class HitTestLocation;

class ClipRect {
  USING_FAST_MALLOC(ClipRect);

 public:
  ClipRect() : has_radius_(false) {}

  ClipRect(const LayoutRect& rect) : rect_(rect), has_radius_(false) {}

  ClipRect(const FloatClipRect& rect)
      : rect_(rect.Rect()), has_radius_(rect.HasRadius()) {}

  void SetRect(const FloatClipRect& rect) {
    rect_ = LayoutRect(rect.Rect());
    has_radius_ = rect.HasRadius();
  }

  const LayoutRect& Rect() const { return rect_; }

  bool HasRadius() const { return has_radius_; }
  void SetHasRadius(bool has_radius) { has_radius_ = has_radius; }

  bool operator==(const ClipRect& other) const {
    return Rect() == other.Rect() && HasRadius() == other.HasRadius();
  }
  bool operator!=(const ClipRect& other) const {
    return Rect() != other.Rect() || HasRadius() != other.HasRadius();
  }
  bool operator!=(const LayoutRect& other_rect) const {
    return Rect() != other_rect;
  }

  void Intersect(const LayoutRect& other) { rect_.Intersect(other); }
  void Intersect(const ClipRect& other) {
    rect_.Intersect(other.Rect());
    if (other.HasRadius())
      has_radius_ = true;
  }
  void Move(const LayoutSize& size) { rect_.Move(size); }
  void Move(const IntSize& size) { rect_.Move(size); }
  void MoveBy(const LayoutPoint& point) { rect_.MoveBy(point); }

  bool IsEmpty() const { return rect_.IsEmpty(); }
  bool Intersects(const HitTestLocation&) const;

  String ToString() const;

 private:
  LayoutRect rect_;
  bool has_radius_;
};

inline ClipRect Intersection(const ClipRect& a, const ClipRect& b) {
  ClipRect c = a;
  c.Intersect(b);
  return c;
}

CORE_EXPORT std::ostream& operator<<(std::ostream&, const ClipRect&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_CLIP_RECT_H_
