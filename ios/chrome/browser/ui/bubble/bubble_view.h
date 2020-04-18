// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_

#import <UIKit/UIKit.h>

// The fixed distance from the leading edge of the bubble to the anchor point if
// leading aligned, and from the trailing edge of the bubble to the anchor point
// if trailing aligned.
extern const CGFloat kBubbleAlignmentOffset;

// Direction for the bubble to point.
typedef NS_ENUM(NSInteger, BubbleArrowDirection) {
  // Bubble is below the target UI element and the arrow is pointing up.
  BubbleArrowDirectionUp,
  // Bubble is above the target UI element and the arrow is pointing down.
  BubbleArrowDirectionDown,
};

// Alignment of the bubble's arrow relative to the rest of the bubble.
typedef NS_ENUM(NSInteger, BubbleAlignment) {
  // Arrow is aligned to the leading edge of the bubble.
  BubbleAlignmentLeading,
  // Arrow is center aligned on the bubble.
  BubbleAlignmentCenter,
  // Arrow is aligned to the trailing edge of the bubble.
  BubbleAlignmentTrailing,
};

// Speech bubble shaped view that displays a message.
@interface BubbleView : UIView

// Initialize with the given text, direction that the bubble should point, and
// alignment of the bubble.
- (instancetype)initWithText:(NSString*)text
              arrowDirection:(BubbleArrowDirection)direction
                   alignment:(BubbleAlignment)alignment
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;

- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_H_
