// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/signed_in_sync_in_progress_view.h"

#import "ios/chrome/browser/ui/material_components/activity_indicator.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MaterialActivityIndicator.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 48;

}  // anonymous namespace

@implementation SignedInSyncInProgressView

- (instancetype)initWithFrame:(CGRect)aRect {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    MDCActivityIndicator* activityIndicator;
    UIImageView* icon;
    UILabel* label;

    icon = [[UIImageView alloc] initWithImage:nil];
    [icon setTranslatesAutoresizingMaskIntoConstraints:NO];
    [icon setImage:
              [[UIImage imageNamed:@"ntp_opentabs_laptop"]
                  imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]];

    label = [[UILabel alloc] initWithFrame:CGRectZero];
    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [label setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
    [label setText:l10n_util::GetNSString(IDS_IOS_RECENT_TABS_OTHER_DEVICES)];
    [label setBackgroundColor:[UIColor whiteColor]];

    activityIndicator = [[MDCActivityIndicator alloc] initWithFrame:CGRectZero];
    [activityIndicator setCycleColors:ActivityIndicatorBrandedCycleColors()];
    [activityIndicator setTranslatesAutoresizingMaskIntoConstraints:NO];
    [activityIndicator startAnimating];

    self.tintColor = recent_tabs::GetIconColorGray();
    self.isAccessibilityElement = YES;
    self.accessibilityLabel = [label accessibilityLabel];
    self.accessibilityTraits |=
        UIAccessibilityTraitButton | UIAccessibilityTraitHeader;

    [self addSubview:icon];
    [self addSubview:label];
    [self addSubview:activityIndicator];

    NSDictionary* viewsDictionary = @{
      @"icon" : icon,
      @"label" : label,
      @"activityIndicator" : activityIndicator,
    };
    NSArray* constraints = @[
      @"H:|-16-[icon]-16-[label]-(>=16)-[activityIndicator(16)]-20-|",
      @"V:|-12-[label]-12-|",
      @"V:[activityIndicator(16)]",
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:activityIndicator
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:icon
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:label
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
  }
  return self;
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kDesiredHeight;
}

@end
