// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_INFO_BUBBLE_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

namespace info_bubble {

// These values are in view coordinates.
const CGFloat kBubbleArrowHeight = 8.0;
const CGFloat kBubbleArrowWidth = 15.0;
const CGFloat kBubbleCornerRadius = 2.0;
const CGFloat kBubbleArrowXOffset = kBubbleArrowWidth + kBubbleCornerRadius;

// Constants that define where the bubble will have rounded corners.
enum CornerFlags {
  kRoundedTopCorners = 1,
  kRoundedBottomCorners = 1 << 1,
  kRoundedAllCorners = kRoundedTopCorners | kRoundedBottomCorners,
};

enum BubbleArrowLocation {
  kTopLeading,
  kTopCenter,
  kTopTrailing,
  kNoArrow,
};

enum BubbleAlignment {
  // The tip of the arrow points to the anchor point.
  kAlignArrowToAnchor,
  // The edge nearest to the arrow is lined up with the anchor point.
  kAlignEdgeToAnchorEdge,
  // Align the trailing edge (right in LTR, left in RTL) to the anchor point.
  kAlignTrailingEdgeToAnchorEdge,
  // Align the leading edge (left in LTR, right in RTL)  to the anchor point.
  kAlignLeadingEdgeToAnchorEdge,
};

}  // namespace info_bubble

// Content view for a bubble with an arrow showing arbitrary content.
// This is where nonrectangular drawing happens.
@interface InfoBubbleView : NSView {
 @private
  info_bubble::BubbleArrowLocation arrowLocation_;
  info_bubble::BubbleAlignment alignment_;
  info_bubble::CornerFlags cornerFlags_;
  base::scoped_nsobject<NSColor> backgroundColor_;
}

@property(assign, nonatomic) info_bubble::BubbleArrowLocation arrowLocation;
@property(assign, nonatomic) info_bubble::BubbleAlignment alignment;
@property(assign, nonatomic) info_bubble::CornerFlags cornerFlags;

// Returns the point location in view coordinates of the tip of the arrow.
- (NSPoint)arrowTip;

// Gets and sets the bubble's background color.
- (NSColor*)backgroundColor;
- (void)setBackgroundColor:(NSColor*)backgroundColor;

@end

#endif  // CHROME_BROWSER_UI_COCOA_INFO_BUBBLE_VIEW_H_
