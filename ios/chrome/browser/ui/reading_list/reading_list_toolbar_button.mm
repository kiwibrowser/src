// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_toolbar_button.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ReadingListToolbarButton ()

// The actual UIButton object inside ReadingListToolbarButton.
@property(nonatomic, strong) UIButton* button;

// Width constraint for the ReadingListToolbarButton.
@property(nonatomic, strong) NSLayoutConstraint* widthConstraint;

@end

@implementation ReadingListToolbarButton

@synthesize button = _button;
@synthesize widthConstraint = _widthConstraint;

#pragma mark - Public

- (instancetype)initWithText:(NSString*)labelText
                 destructive:(BOOL)isDestructive
                    position:(ButtonPositioning)position {
  self = [super init];
  if (!self) {
    return self;
  }

  _button = [self buttonWithText:labelText
                     destructive:isDestructive
                        position:position];
  _button.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:_button];

  NSDictionary* views = @{@"button" : _button};
  NSArray* constraints = nil;

  switch (position) {
    case Leading: {
      constraints = @[ @"V:|[button]|", @"H:|[button]" ];
      ApplyVisualConstraints(constraints, views);
      [_button.trailingAnchor
          constraintLessThanOrEqualToAnchor:self.trailingAnchor]
          .active = YES;
      break;
    }
    case Centered: {
      constraints = @[ @"V:|[button]|" ];
      ApplyVisualConstraints(constraints, views);
      [_button.centerXAnchor constraintEqualToAnchor:self.centerXAnchor]
          .active = YES;
      [_button.trailingAnchor
          constraintLessThanOrEqualToAnchor:self.trailingAnchor]
          .active = YES;
      [_button.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.leadingAnchor]
          .active = YES;
      break;
    }
    case Trailing: {
      constraints = @[ @"V:|[button]|", @"H:[button]|" ];
      ApplyVisualConstraints(constraints, views);
      [_button.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.leadingAnchor]
          .active = YES;
      break;
    }
  }
  return self;
}

- (void)addTarget:(id)target
              action:(SEL)action
    forControlEvents:(UIControlEvents)controlEvents {
  [self.button addTarget:target action:action forControlEvents:controlEvents];
}

- (void)setTitle:(NSString*)title {
  [self.button setTitle:title forState:UIControlStateNormal];
}

- (void)setEnabled:(BOOL)enabled {
  self.button.enabled = enabled;
}

- (UILabel*)titleLabel {
  return self.button.titleLabel;
}

- (void)setMaxWidth:(CGFloat)maxWidth {
  if (!self.widthConstraint) {
    self.widthConstraint =
        [self.button.widthAnchor constraintLessThanOrEqualToConstant:maxWidth];
    self.widthConstraint.active = YES;
  }
  self.widthConstraint.constant = maxWidth;
}

#pragma mark - Private

- (UIButton*)buttonWithText:(NSString*)title
                destructive:(BOOL)isDestructive
                   position:(ButtonPositioning)position {
  UIButton* button = [UIButton buttonWithType:UIButtonTypeCustom];
  [button setTitle:title forState:UIControlStateNormal];
  button.titleLabel.numberOfLines = 3;
  button.titleLabel.adjustsFontSizeToFitWidth = YES;

  button.backgroundColor = [UIColor whiteColor];
  UIColor* textColor = isDestructive ? [[MDCPalette cr_redPalette] tint500]
                                     : [[MDCPalette cr_bluePalette] tint500];
  [button setTitleColor:textColor forState:UIControlStateNormal];
  [button setTitleColor:[UIColor lightGrayColor]
               forState:UIControlStateDisabled];
  [button setTitleColor:[textColor colorWithAlphaComponent:0.3]
               forState:UIControlStateHighlighted];
  [[button titleLabel] setFont:[[self class] textFont]];

  switch (position) {
    case Leading: {
      button.titleLabel.textAlignment = NSTextAlignmentNatural;
      if (UseRTLLayout()) {
        button.contentHorizontalAlignment =
            UIControlContentHorizontalAlignmentRight;
      } else {
        button.contentHorizontalAlignment =
            UIControlContentHorizontalAlignmentLeft;
      }
      break;
    }
    case Centered: {
      button.titleLabel.textAlignment = NSTextAlignmentCenter;
      button.contentHorizontalAlignment =
          UIControlContentHorizontalAlignmentCenter;
      break;
    }
    case Trailing: {
      if (UseRTLLayout()) {
        button.titleLabel.textAlignment = NSTextAlignmentLeft;
        button.contentHorizontalAlignment =
            UIControlContentHorizontalAlignmentLeft;
      } else {
        button.titleLabel.textAlignment = NSTextAlignmentRight;
        button.contentHorizontalAlignment =
            UIControlContentHorizontalAlignmentRight;
      }
      break;
    }
  }

  return button;
}

#pragma mark - Static Properties

// The font of the title text.
+ (UIFont*)textFont {
  return [MDCTypography subheadFont];
}

@end
