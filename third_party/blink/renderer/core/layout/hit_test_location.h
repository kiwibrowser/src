/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_LOCATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_LOCATION_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/geometry/float_quad.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class FloatRoundedRect;

class CORE_EXPORT HitTestLocation {
  DISALLOW_NEW();

 public:
  // Note that all points are in contents (aka "page") coordinate space for the
  // document that is being hit tested.
  HitTestLocation();
  HitTestLocation(const LayoutPoint&);
  HitTestLocation(const FloatPoint&);
  HitTestLocation(const FloatPoint&, const FloatQuad&);
  // Pass non-zero padding values to perform a rect-based hit test.
  HitTestLocation(const LayoutPoint& center_point,
                  const LayoutRectOutsets& padding);
  HitTestLocation(const HitTestLocation&, const LayoutSize& offset);
  HitTestLocation(const HitTestLocation&);
  ~HitTestLocation();
  HitTestLocation& operator=(const HitTestLocation&);

  const LayoutPoint& Point() const { return point_; }
  IntPoint RoundedPoint() const { return RoundedIntPoint(point_); }

  // Rect-based hit test related methods.
  bool IsRectBasedTest() const { return is_rect_based_; }
  bool IsRectilinear() const { return is_rectilinear_; }
  const LayoutRect& BoundingBox() const { return bounding_box_; }
  IntRect EnclosingIntRect() const {
    return ::blink::EnclosingIntRect(bounding_box_);
  }

  // Returns the 1px x 1px hit test rect for a point.
  // TODO(pdr): Use a 0px x 0px rect for point-based tests and switch to
  // inclusive intersection checks which work with empty rects. Rect-based hit
  // testing is used even for point-based tests so a non-empty rect is currently
  // needed for LayoutRect::Intersects (see |HitTestLocation::IntersectsRect|).
  static LayoutRect RectForPoint(const LayoutPoint& point) {
    return LayoutRect(FlooredIntPoint(point), IntSize(1, 1));
  }
  static LayoutRect RectForPoint(const LayoutPoint& point,
                                 const LayoutRectOutsets& padding) {
    LayoutRect rect = RectForPoint(point);
    rect.ExpandEdges(padding.Top(), padding.Right(), padding.Bottom(),
                     padding.Left());
    return rect;
  }

  bool Intersects(const LayoutRect&) const;
  bool Intersects(const FloatRect&) const;
  bool Intersects(const FloatRoundedRect&) const;
  bool ContainsPoint(const FloatPoint&) const;

  const FloatPoint& TransformedPoint() const { return transformed_point_; }
  const FloatQuad& TransformedRect() const { return transformed_rect_; }

 private:
  template <typename RectType>
  bool IntersectsRect(const RectType&, const RectType& bounding_box) const;
  void Move(const LayoutSize& offset);

  // These are cached forms of the more accurate |transformed_point_| and
  // |transformed_rect_|, below.
  LayoutPoint point_;
  LayoutRect bounding_box_;

  FloatPoint transformed_point_;
  FloatQuad transformed_rect_;

  bool is_rect_based_;
  bool is_rectilinear_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_HIT_TEST_LOCATION_H_
