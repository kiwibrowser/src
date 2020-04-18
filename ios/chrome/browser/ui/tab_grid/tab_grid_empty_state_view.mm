// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/tab_grid_empty_state_view.h"

#import "ios/chrome/browser/ui/tab_grid/tab_grid_constants.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridEmptyStateView ()
@property(nonatomic, copy, readonly) NSString* title;
@property(nonatomic, copy, readonly) NSString* body;
@property(nonatomic, strong) NSArray* centeredConstraints;
@property(nonatomic, strong) NSArray* trailingAlignedConstraints;
@end

@implementation TabGridEmptyStateView
@synthesize title = _title;
@synthesize body = _body;
@synthesize centeredConstraints = _centeredConstraints;
@synthesize trailingAlignedConstraints = _trailingAlignedConstraints;

- (instancetype)initWithPage:(TabGridPage)page {
  if (self = [super initWithFrame:CGRectZero]) {
    switch (page) {
      case TabGridPageIncognitoTabs:
        _title = l10n_util::GetNSString(
            IDS_IOS_TAB_GRID_INCOGNITO_TABS_EMPTY_STATE_TITLE);
        _body = l10n_util::GetNSString(
            IDS_IOS_TAB_GRID_INCOGNITO_TABS_EMPTY_STATE_BODY);
        break;
      case TabGridPageRegularTabs:
        _title = l10n_util::GetNSString(
            IDS_IOS_TAB_GRID_REGULAR_TABS_EMPTY_STATE_TITLE);
        _body = l10n_util::GetNSString(
            IDS_IOS_TAB_GRID_REGULAR_TABS_EMPTY_STATE_BODY);
        break;
      case TabGridPageRemoteTabs:
        // No-op. Empty page.
        break;
    }
  }
  return self;
}

#pragma mark - UIView

- (void)willMoveToSuperview:(UIView*)newSuperview {
  // The first time this moves to a superview, perform the view setup.
  if (newSuperview && self.subviews.count == 0) {
    [self setupViews];
  }
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  if (self.traitCollection.verticalSizeClass ==
          UIUserInterfaceSizeClassRegular &&
      self.traitCollection.horizontalSizeClass ==
          UIUserInterfaceSizeClassCompact) {
    // The only centered configuration is when the UI is narrow but
    // vertically long.
    [NSLayoutConstraint deactivateConstraints:self.trailingAlignedConstraints];
    [NSLayoutConstraint activateConstraints:self.centeredConstraints];
  } else {
    [NSLayoutConstraint deactivateConstraints:self.centeredConstraints];
    [NSLayoutConstraint activateConstraints:self.trailingAlignedConstraints];
  }
}

#pragma mark - Private

- (void)setupViews {
  UILabel* topLabel = [[UILabel alloc] init];
  topLabel.translatesAutoresizingMaskIntoConstraints = NO;
  topLabel.text = self.title;
  topLabel.textColor = UIColorFromRGB(kTabGridEmptyStateTitleTextColor);
  topLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle2];
  topLabel.adjustsFontForContentSizeCategory = YES;
  topLabel.numberOfLines = 0;
  topLabel.textAlignment = NSTextAlignmentCenter;
  [self addSubview:topLabel];
  UILabel* bottomLabel = [[UILabel alloc] init];
  bottomLabel.translatesAutoresizingMaskIntoConstraints = NO;
  bottomLabel.text = self.body;
  bottomLabel.textColor = UIColorFromRGB(kTabGridEmptyStateBodyTextColor);
  bottomLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  bottomLabel.adjustsFontForContentSizeCategory = YES;
  bottomLabel.numberOfLines = 0;
  bottomLabel.textAlignment = NSTextAlignmentCenter;
  [self addSubview:bottomLabel];

  self.centeredConstraints = @[
    [topLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [topLabel.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [topLabel.bottomAnchor
        constraintEqualToAnchor:self.centerYAnchor
                       constant:-kTabGridEmptyStateVerticalMargin / 2.0f],
    [bottomLabel.topAnchor
        constraintEqualToAnchor:self.centerYAnchor
                       constant:kTabGridEmptyStateVerticalMargin / 2.0f],
    [bottomLabel.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [bottomLabel.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
  ];
  self.trailingAlignedConstraints = @[
    [bottomLabel.trailingAnchor
        constraintEqualToAnchor:self.trailingAnchor
                       constant:-kTabGridEmptyStateHorizontalInset],
    [bottomLabel.bottomAnchor
        constraintEqualToAnchor:self.bottomAnchor
                       constant:-kTabGridEmptyStateVerticalInset],
    [bottomLabel.topAnchor
        constraintEqualToAnchor:topLabel.bottomAnchor
                       constant:kTabGridEmptyStateVerticalMargin],
    [bottomLabel.trailingAnchor
        constraintEqualToAnchor:topLabel.trailingAnchor],
  ];
}

@end
