// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/card_stack_pinch_gesture_recognizer.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CardStackPinchGestureRecognizer ()

// Returns the number of non-ended, non-cancelled touches in |event|.
- (NSUInteger)numberOfActiveTouchesInEvent:(UIEvent*)event;

@end

@implementation CardStackPinchGestureRecognizer

@synthesize numberOfActiveTouches = numberOfActiveTouches_;

- (NSUInteger)numberOfActiveTouches {
  // Certain corner cases can cause |numberOfActiveTouches_| to temporarily
  // grow to be larger than |[self numberOfTouches]| (this seems to occur only
  // when the user brings two fingers very closely together). As a safeguard,
  // ensure that the number returned is never greater than
  // |[self numberOfTouches]|, as the latter number dictates the greatest index
  // that a client can pass to |locationOfTouch:| without causing an exception.
  return std::min(numberOfActiveTouches_, [self numberOfTouches]);
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  numberOfActiveTouches_ = [self numberOfActiveTouchesInEvent:event];
  [super touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  numberOfActiveTouches_ = [self numberOfActiveTouchesInEvent:event];
  [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  numberOfActiveTouches_ = [self numberOfActiveTouchesInEvent:event];
  [super touchesCancelled:touches withEvent:event];
}

- (void)reset {
  numberOfActiveTouches_ = 0;
  [super reset];
}

- (NSUInteger)numberOfActiveTouchesInEvent:(UIEvent*)event {
  NSUInteger count = 0;
  for (UITouch* touch in [event touchesForGestureRecognizer:self]) {
    if (touch.phase == UITouchPhaseBegan || touch.phase == UITouchPhaseMoved ||
        touch.phase == UITouchPhaseStationary)
      count++;
  }
  return count;
}

@end
