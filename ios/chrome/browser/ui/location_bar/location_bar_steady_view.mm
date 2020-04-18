// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/location_bar/location_bar_steady_view.h"

#import "ios/chrome/browser/ui/location_bar/extended_touch_target_button.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Length of the trailing button side.
const CGFloat kButtonSize = 20;

const CGFloat kButtonTrailingSpacing = 10;

}  // namespace

@interface LocationBarSteadyView ()

// Constraints to hide the trailing button.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* hideButtonConstraints;

// Constraints to show the trailing button.
@property(nonatomic, strong)
    NSArray<NSLayoutConstraint*>* showButtonConstraints;

@end

@implementation LocationBarSteadyView
@synthesize locationLabel = _locationLabel;
@synthesize locationIconImageView = _locationIconImageView;
@synthesize trailingButton = _trailingButton;
@synthesize hideButtonConstraints = _hideButtonConstraints;
@synthesize showButtonConstraints = _showButtonConstraints;

- (instancetype)init {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _locationLabel = [[UILabel alloc] init];
    _locationIconImageView = [[UIImageView alloc] init];
    _locationIconImageView.translatesAutoresizingMaskIntoConstraints = NO;
    [_locationIconImageView
        setContentCompressionResistancePriority:UILayoutPriorityRequired
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];

    // Setup trailing button.
    _trailingButton = [[ExtendedTouchTargetButton alloc] init];
    _trailingButton.translatesAutoresizingMaskIntoConstraints = NO;

    // Setup label.
    _locationLabel.lineBreakMode = NSLineBreakByTruncatingHead;
    _locationLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [_locationLabel
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:UILayoutConstraintAxisVertical];

    // Container for location label and icon.
    UIView* container = [[UIView alloc] init];
    container.translatesAutoresizingMaskIntoConstraints = NO;
    container.userInteractionEnabled = NO;
    [container addSubview:_locationIconImageView];
    [container addSubview:_locationLabel];

    [self addSubview:_trailingButton];
    [self addSubview:container];

    ApplyVisualConstraints(
        @[ @"|[icon][label]|", @"V:|[label]|", @"V:|[container]|" ], @{
          @"icon" : _locationIconImageView,
          @"label" : _locationLabel,
          @"button" : _trailingButton,
          @"container" : container
        });

    AddSameCenterYConstraint(_locationIconImageView, container);

    // Make the label graviatate towards the center of the view.
    NSLayoutConstraint* centerX =
        [container.centerXAnchor constraintEqualToAnchor:self.centerXAnchor];
    centerX.priority = UILayoutPriorityDefaultHigh;

    [NSLayoutConstraint activateConstraints:@[
      [container.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.leadingAnchor],
      [_trailingButton.centerYAnchor
          constraintEqualToAnchor:self.centerYAnchor],
      [container.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
      [_trailingButton.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:container.trailingAnchor],
      centerX,
    ]];

    // Setup hiding constraints.
    _hideButtonConstraints = @[
      [_trailingButton.widthAnchor constraintEqualToConstant:0],
      [_trailingButton.heightAnchor constraintEqualToConstant:0],
      [self.trailingButton.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor]
    ];

    // Setup and activate the show button constraints.
    _showButtonConstraints = @[
      // TODO(crbug.com/821804) Replace the temorary size when the icon is
      // available.
      [_trailingButton.widthAnchor constraintEqualToConstant:kButtonSize],
      [_trailingButton.heightAnchor constraintEqualToConstant:kButtonSize],
      [self.trailingButton.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor
                         constant:-kButtonTrailingSpacing],
    ];
    [NSLayoutConstraint activateConstraints:_showButtonConstraints];
  }
  return self;
}

- (void)hideButton:(BOOL)hidden {
  if (hidden) {
    [NSLayoutConstraint deactivateConstraints:self.showButtonConstraints];
    [NSLayoutConstraint activateConstraints:self.hideButtonConstraints];
  } else {
    [NSLayoutConstraint deactivateConstraints:self.hideButtonConstraints];
    [NSLayoutConstraint activateConstraints:self.showButtonConstraints];
  }
}

@end
