// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_button.h"

#import "ios/third_party/material_components_ios/src/components/Ink/src/MaterialInk.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabSwitcherButton () {
  MDCInkTouchController* _inkTouchController;
}
@end

@implementation TabSwitcherButton

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _inkTouchController = [[MDCInkTouchController alloc] initWithView:self];
    [_inkTouchController addInkView];
    // TODO(crbug.com/606807): Adjust the desired ink color.
    [_inkTouchController defaultInkView].inkColor =
        [[UIColor whiteColor] colorWithAlphaComponent:0.4];
  }
  return self;
}

- (void)resetState {
  [_inkTouchController cancelInkTouchProcessing];
}

@end
