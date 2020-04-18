// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/contextual_search/window_gesture_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation WindowGestureObserver {
  NSObject* _target;
  SEL _action;
  BOOL _actionPassesSelf;
}

@synthesize viewToExclude = _viewToExclude;
@synthesize touchedView = _touchedView;

- (instancetype)initWithTarget:(id)target action:(SEL)action {
  if ((self = [super initWithTarget:target action:action])) {
    _target = static_cast<NSObject*>(target);
    _action = action;
    NSMethodSignature* signature = [_target methodSignatureForSelector:action];
    _actionPassesSelf = [signature numberOfArguments] == 3;
    [self removeTarget:target action:action];
    self.cancelsTouchesInView = NO;
  }
  return self;
}

- (void)setViewToExclude:(UIView*)viewToExclude {
  _viewToExclude = viewToExclude;
  for (UIGestureRecognizer* recognizer in [viewToExclude gestureRecognizers]) {
    [self requireGestureRecognizerToFail:recognizer];
  }
}

- (void)addTarget:(id)target action:(SEL)action {
  // No-op.
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  _touchedView = nil;
  for (UITouch* touch in touches) {
    if (![[touch view] isDescendantOfView:_viewToExclude]) {
      _touchedView = [touch view];
      dispatch_async(dispatch_get_main_queue(), ^{

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        if (_actionPassesSelf) {
          [_target performSelector:_action withObject:self];
        } else {
          [_target performSelector:_action];
        }
#pragma clang diagnostic pop

      });
      // Only invoke from the first qualifying touch.
      break;
    }
  }

  // Cancels to forward touch to other handlers.
  self.state = UIGestureRecognizerStateFailed;
  [super touchesBegan:touches withEvent:event];
}

@end
