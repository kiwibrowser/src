// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_picker_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Ink/src/MaterialInk.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kIdentityPickerViewRadius = 8.;
// Sizes.
const CGFloat kArrowDownSize = 24.;
// Distances/margins.
const CGFloat kArrowDownMargin = 12.;
const CGFloat kHorizontalAvatarMargin = 16.;
const CGFloat kVerticalMargin = 12.;
// Colors
const int kHeaderBackgroundColor = 0xf1f3f4;

}  // namespace

@interface IdentityPickerView ()

@property(nonatomic, strong) IdentityView* identityView;
@property(nonatomic, strong) MDCInkView* inkView;

@end

@implementation IdentityPickerView

@synthesize identityView = _identityView;
@synthesize inkView = _inkView;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.layer.cornerRadius = kIdentityPickerViewRadius;
    self.backgroundColor = UIColorFromRGB(kHeaderBackgroundColor);
    // Adding view elements inside.
    // Ink view.
    _inkView = [[MDCInkView alloc] initWithFrame:CGRectZero];
    _inkView.layer.cornerRadius = kIdentityPickerViewRadius;
    _inkView.inkStyle = MDCInkStyleBounded;
    _inkView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_inkView];

    // Arrow down.
    UIImageView* arrowDownImageView =
        [[UIImageView alloc] initWithFrame:CGRectZero];
    arrowDownImageView.translatesAutoresizingMaskIntoConstraints = NO;
    arrowDownImageView.image =
        [UIImage imageNamed:@"identity_picker_view_arrow_down"];
    [self addSubview:arrowDownImageView];

    // Main view with avatar, name and email.
    _identityView = [[IdentityView alloc] initWithFrame:CGRectZero];
    _identityView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_identityView];

    // Layout constraints.
    NSDictionary* views = @{
      @"identityview" : _identityView,
      @"arrow" : arrowDownImageView,
    };
    NSDictionary* metrics = @{
      @"ArrowMargin" : @(kArrowDownMargin),
      @"ArrowSize" : @(kArrowDownSize),
      @"HAvatMrg" : @(kHorizontalAvatarMargin),
      @"VMargin" : @(kVerticalMargin),
    };
    NSArray* constraints = @[
      // Horizontal constraints.
      @"H:|[identityview]-(ArrowMargin)-[arrow]-(ArrowMargin)-|",
      // Vertical constraints.
      @"V:|[identityview]|",
      // Size constraints.
      @"H:[arrow(ArrowSize)]",
      @"V:[arrow(ArrowSize)]",
    ];
    AddSameCenterYConstraint(self, arrowDownImageView);
    ApplyVisualConstraintsWithMetrics(constraints, views, metrics);
  }
  return self;
}

#pragma mark - Setter

- (void)setIdentityAvatar:(UIImage*)identityAvatar {
  [self.identityView setAvatar:identityAvatar];
}

- (void)setIdentityName:(NSString*)name email:(NSString*)email {
  DCHECK(email);
  if (!name.length) {
    [self.identityView setTitle:email subtitle:nil];
  } else {
    [self.identityView setTitle:name subtitle:email];
  }
}

#pragma mark - Private

- (CGPoint)locationFromTouches:(NSSet*)touches {
  UITouch* touch = [touches anyObject];
  return [touch locationInView:self];
}

#pragma mark - UIResponder

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesBegan:touches withEvent:event];
  CGPoint location = [self locationFromTouches:touches];
  [self.inkView startTouchBeganAnimationAtPoint:location completion:nil];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesEnded:touches withEvent:event];
  CGPoint location = [self locationFromTouches:touches];
  [self.inkView startTouchEndedAnimationAtPoint:location completion:nil];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  [super touchesCancelled:touches withEvent:event];
  CGPoint location = [self locationFromTouches:touches];
  [self.inkView startTouchEndedAnimationAtPoint:location completion:nil];
}

@end
