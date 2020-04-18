// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/show_full_history_view.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#import "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Desired height of the view.
const CGFloat kDesiredHeight = 48;

}  // namespace

@implementation ShowFullHistoryView

- (instancetype)initWithFrame:(CGRect)aRect {
  self = [super initWithFrame:aRect];
  if (self) {
    UIImageView* icon = [[UIImageView alloc] initWithImage:nil];
    [icon setTranslatesAutoresizingMaskIntoConstraints:NO];
    [icon setImage:[UIImage imageNamed:@"ntp_opentabs_clock"]];

    UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [label setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
    [label setTextAlignment:NSTextAlignmentNatural];
    [label setTextColor:recent_tabs::GetTextColorGray()];
    [label setText:l10n_util::GetNSString(IDS_HISTORY_SHOWFULLHISTORY_LINK)];
    [label setBackgroundColor:[UIColor whiteColor]];

    [self addSubview:icon];
    [self addSubview:label];

    self.isAccessibilityElement = YES;
    self.accessibilityLabel = label.text;
    self.accessibilityTraits |= UIAccessibilityTraitButton;

    NSDictionary* viewsDictionary = @{
      @"icon" : icon,
      @"label" : label,
    };
    NSArray* constraints =
        @[ @"H:|-56-[icon(==16)]-16-[label]-(>=16)-|", @"V:[icon(==16)]" ];
    ApplyVisualConstraints(constraints, viewsDictionary);
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:icon
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
                                     attribute:NSLayoutAttributeCenterY
                                    multiplier:1.0
                                      constant:0]];
    [self addConstraint:[NSLayoutConstraint
                            constraintWithItem:label
                                     attribute:NSLayoutAttributeCenterY
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:self
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
