// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import "chrome/browser/ui/cocoa/test/styled_text_field_test_helper.h"

@implementation StyledTextFieldTestCell
@synthesize leftMargin = leftMargin_;
@synthesize rightMargin = rightMargin_;

- (NSRect)textFrameForFrame:(NSRect)frame {
  NSRect textFrame = [super textFrameForFrame:frame];
  textFrame.origin.x += leftMargin_;
  textFrame.size.width -= (leftMargin_ + rightMargin_);
  return textFrame;
}
@end
