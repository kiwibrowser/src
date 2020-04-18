// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_ui_bar_button_item.h"

#import "ios/chrome/browser/ui/toolbar/keyboard_assist/toolbar_assistive_keyboard_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ToolbarUIBarButtonItem () {
  id<ToolbarAssistiveKeyboardDelegate> _delegate;
}
@end

@implementation ToolbarUIBarButtonItem

- (instancetype)initWithTitle:(NSString*)title
                     delegate:(id<ToolbarAssistiveKeyboardDelegate>)delegate {
  self = [super initWithTitle:title
                        style:UIBarButtonItemStylePlain
                       target:self
                       action:@selector(pressed)];
  if (self) {
    _delegate = delegate;
  }
  return self;
}

- (void)pressed {
  [_delegate keyPressed:self.title];
}

@end
