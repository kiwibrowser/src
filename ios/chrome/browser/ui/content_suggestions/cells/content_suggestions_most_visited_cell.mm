// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_cell.h"

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_constants.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kIconSizeLegacy = 48;

}  // namespace

@implementation ContentSuggestionsMostVisitedCell : MDCCollectionViewCell

@synthesize faviconView = _faviconView;
@synthesize titleLabel = _titleLabel;

#pragma mark - Public

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _titleLabel = [[UILabel alloc] init];
    if (IsUIRefreshPhase1Enabled()) {
      _titleLabel.textColor = [UIColor colorWithWhite:0 alpha:kTitleAlpha];
      _titleLabel.font =
          [UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    } else {
      _titleLabel.textColor =
          [UIColor colorWithWhite:kLabelTextColor alpha:1.0];
      _titleLabel.font = [MDCTypography captionFont];
    }
    _titleLabel.textAlignment = NSTextAlignmentCenter;
    _titleLabel.preferredMaxLayoutWidth = [[self class] defaultSize].width;
    _titleLabel.numberOfLines = kLabelNumLines;

    _faviconView = [[FaviconViewNew alloc] init];
    if (IsUIRefreshPhase1Enabled()) {
      _faviconView.font =
          [UIFont preferredFontForTextStyle:UIFontTextStyleTitle2];
    } else {
      _faviconView.font = [MDCTypography headlineFont];
    }

    _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _faviconView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:_titleLabel];

    UIView* containerView = nil;
    if (IsUIRefreshPhase1Enabled()) {
      UIImageView* faviconContainer =
          [[UIImageView alloc] initWithFrame:self.bounds];
      faviconContainer.translatesAutoresizingMaskIntoConstraints = NO;
      faviconContainer.image = [UIImage imageNamed:@"ntp_most_visited_tile"];
      [self.contentView addSubview:faviconContainer];
      [faviconContainer addSubview:_faviconView];

      [NSLayoutConstraint activateConstraints:@[
        [faviconContainer.widthAnchor constraintEqualToConstant:kIconSize],
        [faviconContainer.heightAnchor
            constraintEqualToAnchor:faviconContainer.widthAnchor],
        [faviconContainer.centerXAnchor
            constraintEqualToAnchor:_titleLabel.centerXAnchor],
        [_faviconView.heightAnchor constraintEqualToConstant:32],
        [_faviconView.widthAnchor
            constraintEqualToAnchor:_faviconView.heightAnchor],
      ]];
      AddSameCenterConstraints(_faviconView, faviconContainer);
      containerView = faviconContainer;
    } else {
      [self.contentView addSubview:_faviconView];

      [NSLayoutConstraint activateConstraints:@[
        [_faviconView.widthAnchor constraintEqualToConstant:kIconSizeLegacy],
        [_faviconView.heightAnchor
            constraintEqualToAnchor:_faviconView.widthAnchor],
        [_faviconView.centerXAnchor
            constraintEqualToAnchor:_titleLabel.centerXAnchor],
      ]];
      containerView = _faviconView;
    }

    ApplyVisualConstraintsWithMetrics(
        @[ @"V:|[container]-(space)-[title]", @"H:|[title]|" ],
        @{@"container" : containerView, @"title" : _titleLabel},
        @{ @"space" : @(kSpaceIconTitle) });

    self.isAccessibilityElement = YES;
  }
  return self;
}

+ (CGSize)defaultSize {
  return kCellSize;
}

- (CGSize)intrinsicContentSize {
  return [[self class] defaultSize];
}

@end
