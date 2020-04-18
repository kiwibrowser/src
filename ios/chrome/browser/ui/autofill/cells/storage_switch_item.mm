// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/cells/storage_switch_item.h"

#include "components/grit/components_scaled_resources.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Padding used on the leading and trailing edges of the cell.
const CGFloat kHorizontalPadding = 16;
// Padding used on the top and bottom edges of the cell.
const CGFloat kVerticalPadding = 16;
// Space at the leading side of the tooltip button.
const CGFloat kTooltipButtonLeadingSpacing = 8;
// Space at the trailing side of the tooltip button.
const CGFloat kTooltipButtonTrailingSpacing = 30;
}

@implementation StorageSwitchItem

@synthesize on = _on;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [StorageSwitchCell class];
  }
  return self;
}

#pragma mark CollectionViewItem

- (void)configureCell:(StorageSwitchCell*)cell {
  [super configureCell:cell];
  cell.switchView.on = self.on;
}

@end

@implementation StorageSwitchCell

@synthesize textLabel = _textLabel;
@synthesize tooltipButton = _tooltipButton;
@synthesize switchView = _switchView;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    UIView* contentView = self.contentView;

    _textLabel = [[UILabel alloc] init];
    _textLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_textLabel];

    _switchView = [[UISwitch alloc] init];
    _switchView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_switchView];

    _tooltipButton = [UIButton buttonWithType:UIButtonTypeCustom];
    _tooltipButton.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_tooltipButton];

    _textLabel.text = l10n_util::GetNSString(
        IDS_AUTOFILL_CARD_UNMASK_PROMPT_STORAGE_CHECKBOX);
    _textLabel.font = [[MDCTypography fontLoader] mediumFontOfSize:14];
    _textLabel.textColor = [[MDCPalette greyPalette] tint500];
    _textLabel.numberOfLines = 0;
    [_textLabel
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];

    _switchView.onTintColor = [[MDCPalette cr_bluePalette] tint500];

    [_tooltipButton setImage:NativeImage(IDR_AUTOFILL_TOOLTIP_ICON_H)
                    forState:UIControlStateSelected];
    [_tooltipButton setImage:NativeImage(IDR_AUTOFILL_TOOLTIP_ICON)
                    forState:UIControlStateNormal];

    // Set up the constraints.
    [NSLayoutConstraint activateConstraints:@[
      [_textLabel.topAnchor constraintEqualToAnchor:contentView.topAnchor
                                           constant:kVerticalPadding],
      [_textLabel.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor
                                              constant:-kVerticalPadding],
      [_textLabel.leadingAnchor
          constraintEqualToAnchor:contentView.leadingAnchor
                         constant:kHorizontalPadding],
      [_textLabel.trailingAnchor
          constraintLessThanOrEqualToAnchor:_tooltipButton.leadingAnchor
                                   constant:-kTooltipButtonLeadingSpacing],
      [_tooltipButton.centerYAnchor
          constraintEqualToAnchor:contentView.centerYAnchor],
      [_tooltipButton.trailingAnchor
          constraintEqualToAnchor:_switchView.leadingAnchor
                         constant:-kTooltipButtonTrailingSpacing],
      [_switchView.centerYAnchor
          constraintEqualToAnchor:contentView.centerYAnchor],
      [_switchView.trailingAnchor
          constraintEqualToAnchor:contentView.trailingAnchor
                         constant:-kHorizontalPadding],
    ]];
  }
  return self;
}

// Implement -layoutSubviews as per instructions in documentation for
// +[MDCCollectionViewCell cr_preferredHeightForWidth:forItem:].
- (void)layoutSubviews {
  [super layoutSubviews];

  // Adjust the text label preferredMaxLayoutWidth when the parent's width
  // changes, for instance on screen rotation.
  self.textLabel.preferredMaxLayoutWidth =
      CGRectGetWidth(self.contentView.frame) - 2 * kHorizontalPadding -
      kTooltipButtonLeadingSpacing - CGRectGetWidth(self.tooltipButton.frame) -
      kTooltipButtonTrailingSpacing - CGRectGetWidth(self.switchView.frame);

  // Re-layout with the new preferred width to allow the label to adjust its
  // height.
  [super layoutSubviews];
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [self.tooltipButton removeTarget:nil
                            action:nil
                  forControlEvents:self.tooltipButton.allControlEvents];
  [self.switchView removeTarget:nil
                         action:nil
               forControlEvents:self.switchView.allControlEvents];
}

@end
