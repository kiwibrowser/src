// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_PINCH_GESTURE_RECOGNIZER_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_PINCH_GESTURE_RECOGNIZER_H_

#import <UIKit/UIGestureRecognizerSubclass.h>
#import <UIKit/UIKit.h>

// This gesture recognizer maintains state of the pinch with greater detail
// than is available via the interface of a stock UIPinchGestureRecognizer.
@interface CardStackPinchGestureRecognizer : UIPinchGestureRecognizer {
}

// Number of non-cancelled, non-ended touches involved in the gesture.
@property(nonatomic, readonly) NSUInteger numberOfActiveTouches;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_CARD_STACK_PINCH_GESTURE_RECOGNIZER_H_
