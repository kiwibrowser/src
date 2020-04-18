// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

CullRect::CullRect(const CullRect& cull_rect, const IntPoint& offset) {
  rect_ = cull_rect.rect_;
  rect_.MoveBy(offset);
}

CullRect::CullRect(const CullRect& cull_rect, const IntSize& offset) {
  rect_ = cull_rect.rect_;
  rect_.Move(offset);
}

bool CullRect::IntersectsCullRect(const IntRect& bounding_box) const {
  return bounding_box.Intersects(rect_);
}

bool CullRect::IntersectsCullRect(const AffineTransform& transform,
                                  const FloatRect& bounding_box) const {
  return transform.MapRect(bounding_box).Intersects(rect_);
}

bool CullRect::IntersectsCullRect(const LayoutRect& rect_arg) const {
  return rect_.Intersects(EnclosingIntRect(rect_arg));
}

bool CullRect::IntersectsHorizontalRange(LayoutUnit lo, LayoutUnit hi) const {
  return !(lo >= rect_.MaxX() || hi <= rect_.X());
}

bool CullRect::IntersectsVerticalRange(LayoutUnit lo, LayoutUnit hi) const {
  return !(lo >= rect_.MaxY() || hi <= rect_.Y());
}

void CullRect::UpdateCullRect(
    const AffineTransform& local_to_parent_transform) {
  if (rect_ != LayoutRect::InfiniteIntRect())
    rect_ = local_to_parent_transform.Inverse().MapRect(rect_);
}

void CullRect::UpdateForScrollingContents(
    const IntRect& overflow_clip_rect,
    const AffineTransform& local_to_parent_transform) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  rect_.Intersect(overflow_clip_rect);
  UpdateCullRect(local_to_parent_transform);

  // TODO(wangxianzhu, chrishtr): How about non-composited scrolling contents?
  // The distance to expand the cull rect for scrolling contents.
  static const int kPixelDistanceToExpand = 4000;
  rect_.Inflate(kPixelDistanceToExpand);
}

}  // namespace blink
