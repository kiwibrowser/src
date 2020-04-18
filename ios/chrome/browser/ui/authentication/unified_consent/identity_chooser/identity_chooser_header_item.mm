// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_header_item.h"

#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Top margin for the label.
CGFloat kTopMargin = 24.;
// Leading margin for the label.
CGFloat kLeadingMargin = 24.;
// Label font size.
CGFloat kTitleFontSize = 14.;
// Label font color alpha.
CGFloat kFontAlpha = .87;
}  // namespace

@interface IdentityChooserHeaderView : UITableViewHeaderFooterView
@end

@implementation IdentityChooserHeaderView

- (instancetype)initWithReuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithReuseIdentifier:reuseIdentifier];
  if (self) {
    self.contentView.backgroundColor = UIColor.whiteColor;
    UILabel* label = [[UILabel alloc] init];
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.text =
        l10n_util::GetNSString(IDS_IOS_ACCOUNT_IDENTITY_CHOOSER_CHOOSE_ACCOUNT);
    label.font = [UIFont boldSystemFontOfSize:kTitleFontSize];
    label.textColor = [UIColor colorWithWhite:0. alpha:kFontAlpha];
    [self.contentView addSubview:label];
    NSDictionary* views = @{
      @"label" : label,
    };
    NSDictionary* metrics = @{
      @"LeadingMargin" : @(kLeadingMargin),
      @"TopMargin" : @(kTopMargin),
    };
    NSArray* constraints = @[
      // Horitizontal constraints.
      @"H:|-(LeadingMargin)-[label]-(>=0)-|",
      // Vertical constraints.
      @"V:|-(TopMargin)-[label]",
    ];
    ApplyVisualConstraintsWithMetrics(constraints, views, metrics);
  }
  return self;
}

@end

@implementation IdentityChooserHeaderItem

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [IdentityChooserHeaderView class];
  }
  return self;
}

@end
