// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/open_in_toolbar.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The toolbar's open button constants.
const CGFloat kOpenButtonVerticalPadding = 8.0f;
const CGFloat kOpenButtonTrailingPadding = 16.0f;

// The toolbar's border related constants.
const CGFloat kTopBorderHeight = 0.5f;
const CGFloat kTopBorderTransparency = 0.13f;
const int kTopBorderColor = 0x000000;

// The toolbar's background related constants.
const int kToolbarBackgroundColor = 0xFFFFFF;
const CGFloat kToolbarBackgroundTransparency = 0.97f;

}  // anonymous namespace

@interface OpenInToolbar () {
  // Backing object for |self.openButton|.
  MDCButton* _openButton;
  // Backing object for |self.topBorder|.
  UIView* _topBorder;
}

// The "Open in..." button that's hooked up with the target and action passed
// on initialization.
@property(nonatomic, retain, readonly) MDCButton* openButton;

// The line used as the border at the top of the toolbar.
@property(nonatomic, retain, readonly) UIView* topBorder;

@end

@implementation OpenInToolbar

- (instancetype)initWithFrame:(CGRect)aRect {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithTarget:(id)target action:(SEL)action {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK([target respondsToSelector:action]);
    [self setBackgroundColor:UIColorFromRGB(kToolbarBackgroundColor,
                                            kToolbarBackgroundTransparency)];
    [self addSubview:self.openButton];
    [self.openButton addTarget:target
                        action:action
              forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:self.topBorder];
  }
  return self;
}

#pragma mark Accessors

- (MDCButton*)openButton {
  if (!_openButton) {
    _openButton = [[MDCFlatButton alloc] init];
    [_openButton setTitleColor:[[MDCPalette cr_bluePalette] tint500]
                      forState:UIControlStateNormal];
    [_openButton setTitle:l10n_util::GetNSStringWithFixup(IDS_IOS_OPEN_IN)
                 forState:UIControlStateNormal];
    [_openButton sizeToFit];
  }
  return _openButton;
}

- (UIView*)topBorder {
  if (!_topBorder) {
    _topBorder = [[UIView alloc] initWithFrame:CGRectZero];
    [_topBorder setBackgroundColor:UIColorFromRGB(kTopBorderColor,
                                                  kTopBorderTransparency)];
  }
  return _topBorder;
}

#pragma mark Layout

- (void)layoutSubviews {
  [super layoutSubviews];

  // openButton layout.
  CGFloat buttonWidth = CGRectGetWidth(self.openButton.bounds);
  CGFloat buttonHeight = CGRectGetHeight(self.openButton.bounds);

  LayoutRect layout = LayoutRectMake(
      CGRectGetMaxX(self.bounds) - buttonWidth - kOpenButtonTrailingPadding,
      CGRectGetWidth(self.bounds),
      CGRectGetMaxY(self.bounds) - buttonHeight - kOpenButtonVerticalPadding,
      buttonWidth, buttonHeight);
  self.openButton.frame = LayoutRectGetRect(layout);

  // topBorder layout.
  CGRect topBorderFrame = self.bounds;
  topBorderFrame.size.height = kTopBorderHeight;
  self.topBorder.frame = topBorderFrame;
}

- (CGSize)sizeThatFits:(CGSize)size {
  CGSize openButtonSize = [self.openButton sizeThatFits:size];
  CGFloat requiredHeight =
      openButtonSize.height + 2.0 * kOpenButtonVerticalPadding;
  return CGSizeMake(size.width, requiredHeight);
}

@end
