// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/crw_touch_tracking_recognizer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface CRWTouchTrackingRecognizer () <UIGestureRecognizerDelegate> {
  id<CRWTouchTrackingDelegate> __weak _delegate;
}
@end

@implementation CRWTouchTrackingRecognizer

@synthesize touchTrackingDelegate = _delegate;

- (id)initWithDelegate:(id<CRWTouchTrackingDelegate>)delegate {
  if ((self = [super init])) {
    _delegate = delegate;
    self.delegate = self;
  }
  return self;
}

#pragma mark -
#pragma mark UIGestureRecognizer Methods

- (void)reset {
  [super reset];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesBegan:touches withEvent:event];
  [_delegate touched:YES];
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesEnded:touches withEvent:event];
  self.state = UIGestureRecognizerStateFailed;
  [_delegate touched:NO];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesCancelled:touches withEvent:event];
  [_delegate touched:NO];
}

#pragma mark -
#pragma mark UIGestureRecognizerDelegate Method

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  return YES;
}

@end
