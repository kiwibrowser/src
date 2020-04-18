// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_container_view.h"

#import "ios/chrome/browser/ui/animation_util.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_text_field_ios.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#include "skia/ext/skia_utils_ios.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kleadingImageViewEdgeOffset = 9;
// Offset from the leading edge to the textfield when no image is shown.
const CGFloat kTextFieldLeadingOffsetNoImage = 16;
// Space between the leading button and the textfield when a button is shown.
const CGFloat kTextFieldLeadingOffsetImage = 6;
}  // namespace

#pragma mark - OmniboxContainerView

@interface OmniboxContainerView ()
// Constraints the leading textfield side to the leading of |self|.
// Active when the |leadingView| is nil or hidden.
@property(nonatomic, strong) NSLayoutConstraint* leadingTextfieldConstraint;
// When the |leadingImageView| is not hidden, this is a constraint that links
// the leading edge of the button to self leading edge. Used for animations.
@property(nonatomic, strong)
    NSLayoutConstraint* leadingImageViewLeadingConstraint;
// The leading image view. Used for autocomplete icons.
@property(nonatomic, strong) UIImageView* leadingImageView;
// Redefined as readwrite.
@property(nonatomic, strong) OmniboxTextFieldIOS* textField;

@end

@implementation OmniboxContainerView
@synthesize textField = _textField;
@synthesize leadingImageView = _leadingImageView;
@synthesize leadingTextfieldConstraint = _leadingTextfieldConstraint;
@synthesize incognito = _incognito;
@synthesize leadingImageViewLeadingConstraint =
    _leadingImageViewLeadingConstraint;

#pragma mark - Public methods

- (instancetype)initWithFrame:(CGRect)frame
                         font:(UIFont*)font
                    textColor:(UIColor*)textColor
                    tintColor:(UIColor*)tintColor {
  self = [super initWithFrame:frame];
  if (self) {
    _textField = [[OmniboxTextFieldIOS alloc] initWithFrame:frame
                                                       font:font
                                                  textColor:textColor
                                                  tintColor:tintColor];
    [self addSubview:_textField];

    _leadingTextfieldConstraint = [_textField.leadingAnchor
        constraintEqualToAnchor:self.leadingAnchor
                       constant:kTextFieldLeadingOffsetNoImage];

    [NSLayoutConstraint activateConstraints:@[
      [_textField.trailingAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.trailingAnchor],
      [_textField.topAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.topAnchor],
      [_textField.bottomAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.bottomAnchor],
      _leadingTextfieldConstraint,
    ]];

    _textField.translatesAutoresizingMaskIntoConstraints = NO;
    [_textField
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];

    [self createLeadingImageView];
    _leadingImageView.tintColor = tintColor ?: [UIColor blackColor];
  }
  return self;
}

- (void)setLeadingImageHidden:(BOOL)hidden {
  if (hidden) {
    [_leadingImageView removeFromSuperview];
    self.leadingTextfieldConstraint.active = YES;
  } else {
    [self addSubview:_leadingImageView];
    self.leadingTextfieldConstraint.active = NO;
    self.leadingImageViewLeadingConstraint =
        [self.layoutMarginsGuide.leadingAnchor
            constraintEqualToAnchor:self.leadingImageView.leadingAnchor
                           constant:-kleadingImageViewEdgeOffset];

    NSLayoutConstraint* leadingImageViewToTextField = nil;
    leadingImageViewToTextField = [self.leadingImageView.trailingAnchor
        constraintEqualToAnchor:self.textField.leadingAnchor
                       constant:-kTextFieldLeadingOffsetImage];

    [NSLayoutConstraint activateConstraints:@[
      [_leadingImageView.centerYAnchor
          constraintEqualToAnchor:self.layoutMarginsGuide.centerYAnchor],
      self.leadingImageViewLeadingConstraint,
      leadingImageViewToTextField,
    ]];
  }
}

- (void)setLeadingImage:(UIImage*)image {
  [self.leadingImageView setImage:image];
}

- (void)createLeadingImageView {
  _leadingImageView = [[UIImageView alloc] init];
  _leadingImageView.translatesAutoresizingMaskIntoConstraints = NO;
  [_leadingImageView
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [_leadingImageView
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];
  [_leadingImageView
      setContentHuggingPriority:UILayoutPriorityRequired
                        forAxis:UILayoutConstraintAxisHorizontal];
  [_leadingImageView setContentHuggingPriority:UILayoutPriorityRequired
                                       forAxis:UILayoutConstraintAxisVertical];
}

@end
