// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_header_cell.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_session_cell_data.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// View alpha value used when the header cell is not selected.
const CGFloat kInactiveAlpha = 0.54;
const CGFloat kImageViewWidth = 24;
}

@interface TabSwitcherHeaderCell () {
  UIImageView* _imageView;
  UILabel* _label;
}
@end

@implementation TabSwitcherHeaderCell

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.backgroundColor = [[MDCPalette greyPalette] tint900];
    [self loadSubviews];
    [self setSelected:NO];
  }
  return self;
}

+ (NSString*)identifier {
  return @"TabSwitcherHeaderCell";
}

- (NSString*)reuseIdentifier {
  return [[self class] identifier];
}

- (void)loadSessionCellData:(TabSwitcherSessionCellData*)sessionCellData {
  [_imageView setImage:sessionCellData.image];
  [_label setText:sessionCellData.title];
  [self setNeedsLayout];
}

- (void)setSelected:(BOOL)selected {
  [super setSelected:selected];
  [_imageView setAlpha:selected ? 1. : kInactiveAlpha];
  [_label setAlpha:selected ? 1. : kInactiveAlpha];
  [self setNeedsLayout];
}

#pragma mark - Private

- (void)loadSubviews {
  _imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
  [_imageView setContentMode:UIViewContentModeCenter];
  [_imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_imageView setTintColor:[UIColor whiteColor]];
  _label = [[UILabel alloc] initWithFrame:CGRectZero];
  [_label setBackgroundColor:[UIColor clearColor]];
  [_label setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_label setTextColor:[UIColor whiteColor]];
  [_label setFont:[MDCTypography body2Font]];

  // Configure layout.
  // The icon and the title are centered within |contentView|, have a spacing of
  // one-third of icon width and the icon is on the leading side of title.
  UIStackView* stackView =
      [[UIStackView alloc] initWithArrangedSubviews:@[ _imageView, _label ]];
  [stackView setSpacing:kImageViewWidth / 3];
  [self.contentView addSubview:stackView];

  [stackView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [NSLayoutConstraint activateConstraints:@[
    [self.contentView.centerYAnchor
        constraintEqualToAnchor:[stackView centerYAnchor]],
    [self.contentView.centerXAnchor
        constraintEqualToAnchor:[stackView centerXAnchor]]
  ]];
}

@end
