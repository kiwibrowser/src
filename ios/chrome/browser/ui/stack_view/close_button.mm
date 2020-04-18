// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/close_button.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CloseButton {
  __weak id _accessibilityTarget;
  SEL _accessibilityAction;
}

- (void)addAccessibilityElementFocusedTarget:(id)accessibilityTarget
                                      action:(SEL)accessibilityAction {
  DCHECK(!accessibilityTarget ||
         [accessibilityTarget respondsToSelector:accessibilityAction]);
  _accessibilityTarget = accessibilityTarget;
  _accessibilityAction = accessibilityAction;
}

- (void)accessibilityElementDidBecomeFocused {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [_accessibilityTarget performSelector:_accessibilityAction withObject:self];
#pragma clang diagnostic pop
}

@end
