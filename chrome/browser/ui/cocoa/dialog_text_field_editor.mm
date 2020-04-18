// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/dialog_text_field_editor.h"

#import "ui/base/cocoa/touch_bar_forward_declarations.h"

@implementation DialogTextFieldEditor

- (instancetype)init {
  if ((self = [super init])) {
    [self setFieldEditor:YES];
  }
  return self;
}

- (NSTouchBar*)makeTouchBar {
  return nil;
}

@end
