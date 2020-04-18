// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/spacers_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kHeaderDesiredHeightIPhone = 8;
const CGFloat kHeaderDesiredHeightIPad = 48;
const CGFloat kFooterDesiredHeight = 17;

// Text color.
const int kSeparatorColor = 0x5a5a5a;
const CGFloat kSeparatorAlpha = 0.1;

}  // namespace

@implementation RecentlyTabsTopSpacingHeader

+ (CGFloat)desiredHeightInUITableViewCell {
  if (IsIPadIdiom()) {
    return kHeaderDesiredHeightIPad;
  } else {
    return kHeaderDesiredHeightIPhone;
  }
}

@end

@implementation RecentlyClosedSectionFooter

- (instancetype)initWithFrame:(CGRect)aRect {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    // Create and add separator.
    UIView* separator = [[UIView alloc] init];
    [separator setTranslatesAutoresizingMaskIntoConstraints:NO];
    UIColor* separatorColor = UIColorFromRGB(kSeparatorColor, kSeparatorAlpha);
    [separator setBackgroundColor:separatorColor];
    [self addSubview:separator];

    // Set constraints on separator.
    NSDictionary* viewsDictionary = @{ @"separator" : separator };
    // This set of constraints should match the constraints set on the
    // RecentTabsTableViewController.
    // clang-format off
    NSArray* constraints = @[
      @"V:|-(8)-[separator(==1)]",
      @"H:|-(16)-[separator(<=516)]-(16)-|",
      @"H:[separator(==516@500)]"
    ];
    // clang-format on
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:separator
                                     attribute:NSLayoutAttributeCenterX
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterX
                                    multiplier:1
                                      constant:0]];
    ApplyVisualConstraints(constraints, viewsDictionary);
  }
  return self;
}

+ (CGFloat)desiredHeightInUITableViewCell {
  return kFooterDesiredHeight;
}

@end
