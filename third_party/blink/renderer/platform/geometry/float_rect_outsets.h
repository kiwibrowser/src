// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_RECT_OUTSETS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_RECT_OUTSETS_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

// Specifies floating-point lengths to be used to expand a rectangle.
// For example, |top()| returns the distance the top edge should be moved
// upward.
//
// Negative lengths can be used to express insets.
class PLATFORM_EXPORT FloatRectOutsets {
  STACK_ALLOCATED();

 public:
  FloatRectOutsets() : top_(0), right_(0), bottom_(0), left_(0) {}

  FloatRectOutsets(float top, float right, float bottom, float left)
      : top_(top), right_(right), bottom_(bottom), left_(left) {}

  float Top() const { return top_; }
  float Right() const { return right_; }
  float Bottom() const { return bottom_; }
  float Left() const { return left_; }

  void SetTop(float top) { top_ = top; }
  void SetRight(float right) { right_ = right; }
  void SetBottom(float bottom) { bottom_ = bottom; }
  void SetLeft(float left) { left_ = left; }

  // Change outsets to be at least as large as |other|.
  void Unite(const FloatRectOutsets& other);

 private:
  float top_;
  float right_;
  float bottom_;
  float left_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GEOMETRY_FLOAT_RECT_OUTSETS_H_
