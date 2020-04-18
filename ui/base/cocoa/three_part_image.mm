// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/three_part_image.h"

#include "base/logging.h"
#include "ui/base/resource/resource_bundle.h"

namespace ui {

ThreePartImage::ThreePartImage(NSImage* left, NSImage* middle, NSImage* right) {
  DCHECK(left);
  DCHECK(right);
  leftImage_.reset([left retain]);
  rightImage_.reset([right retain]);
  leftSize_ = [leftImage_ size];
  rightSize_ = [rightImage_ size];

  if (middle) {
    middleImage_.reset([middle retain]);
  }
}

ThreePartImage::~ThreePartImage() {
}

NSRect ThreePartImage::GetLeftRect(NSRect bounds) const {
  NSRect left, right;
  NSDivideRect(bounds, &left, &right, leftSize_.width, NSMinXEdge);
  return left;
}

NSRect ThreePartImage::GetMiddleRect(NSRect bounds) const {
  NSRect left, middle, right;
  NSDivideRect(bounds, &left, &middle, leftSize_.width, NSMinXEdge);
  NSDivideRect(middle, &right, &middle, rightSize_.width, NSMaxXEdge);
  return middle;
}

NSRect ThreePartImage::GetRightRect(NSRect bounds) const {
  NSRect left, right;
  NSDivideRect(bounds, &right, &left, rightSize_.width, NSMaxXEdge);
  return right;
}

void ThreePartImage::DrawInRect(NSRect rect,
                                NSCompositingOperation op,
                                CGFloat alpha) const {
  rect.size.height = leftSize_.height;
  NSDrawThreePartImage(rect, leftImage_, middleImage_, rightImage_,
                       NO, op, alpha, NO);
}

BOOL ThreePartImage::HitTest(NSPoint point, NSRect bounds) const {
  NSRect middleRect = GetMiddleRect(bounds);
  if (NSPointInRect(point, middleRect))
    return middleImage_ ? HitTestImage(point, middleImage_, middleRect) : YES;

  NSRect leftRect = GetLeftRect(bounds);
  if (NSPointInRect(point, leftRect))
    return HitTestImage(point, leftImage_, leftRect);

  NSRect rightRect = GetRightRect(bounds);
  if (NSPointInRect(point, rightRect))
    return HitTestImage(point, rightImage_, rightRect);

  return NO;
}

BOOL ThreePartImage::HitTestImage(NSPoint point,
                                  NSImage* image,
                                  NSRect imageRect) const {
  NSRect pointRect = NSMakeRect(point.x, point.y, 1, 1);
  return [image hitTestRect:pointRect
      withImageDestinationRect:imageRect
                       context:nil
                         hints:nil
                       flipped:NO];
}

}  // namespace ui
