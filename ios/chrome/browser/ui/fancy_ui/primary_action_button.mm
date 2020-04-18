// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fancy_ui/primary_action_button.h"

#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation PrimaryActionButton

- (id)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self)
    [self initializeStyling];
  return self;
}

- (id)initWithCoder:(NSCoder*)aDecoder {
  self = [super initWithCoder:aDecoder];
  if (self)
    [self initializeStyling];
  return self;
}

- (void)awakeFromNib {
  [super awakeFromNib];
  [self initializeStyling];
}

- (void)initializeStyling {
  self.hasOpaqueBackground = YES;
  self.underlyingColorHint = [UIColor whiteColor];
  self.inkColor =
      [[[MDCPalette cr_bluePalette] tint300] colorWithAlphaComponent:0.5f];
  [self setBackgroundColor:[[MDCPalette cr_bluePalette] tint500]
                  forState:UIControlStateNormal];
  [self setBackgroundColor:[UIColor colorWithWhite:0.6f alpha:1.0f]
                  forState:UIControlStateDisabled];
  [self setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
}

@end
