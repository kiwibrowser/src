// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/import_data_multiline_detail_cell.h"

#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Padding used on the leading and trailing edges of the cell.
const CGFloat kHorizontalPadding = 16;

// Padding used on the top and bottom edges of the cell.
const CGFloat kVerticalPadding = 16;
}  // namespace

@implementation ImportDataMultilineDetailCell

@synthesize textLabel = _textLabel;
@synthesize detailTextLabel = _detailTextLabel;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.isAccessibilityElement = YES;
    UIView* contentView = self.contentView;

    _textLabel = [[UILabel alloc] init];
    _textLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_textLabel];

    _textLabel.font = [[MDCTypography fontLoader] mediumFontOfSize:14];
    _textLabel.textColor = [[MDCPalette greyPalette] tint900];

    _detailTextLabel = [[UILabel alloc] init];
    _detailTextLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_detailTextLabel];

    _detailTextLabel.font = [[MDCTypography fontLoader] regularFontOfSize:14];
    _detailTextLabel.textColor = [[MDCPalette greyPalette] tint500];
    _detailTextLabel.numberOfLines = 0;

    // Set up the constraints.
    [NSLayoutConstraint activateConstraints:@[
      [_textLabel.leadingAnchor
          constraintEqualToAnchor:contentView.leadingAnchor
                         constant:kHorizontalPadding],
      [_textLabel.trailingAnchor
          constraintEqualToAnchor:contentView.trailingAnchor
                         constant:-kHorizontalPadding],
      [_detailTextLabel.leadingAnchor
          constraintEqualToAnchor:_textLabel.leadingAnchor],
      [_detailTextLabel.trailingAnchor
          constraintEqualToAnchor:_textLabel.trailingAnchor],
      [_textLabel.topAnchor constraintEqualToAnchor:contentView.topAnchor
                                           constant:kVerticalPadding],
      [_textLabel.bottomAnchor
          constraintEqualToAnchor:_detailTextLabel.topAnchor],
      [_detailTextLabel.bottomAnchor
          constraintEqualToAnchor:contentView.bottomAnchor
                         constant:-kVerticalPadding],
    ]];
  }
  return self;
}

- (void)layoutSubviews {
  // When the accessory type is None, the content view of the cell (and thus)
  // the labels inside it span larger than when there is a Checkmark accessory
  // type. That means that toggling the accessory type can induce a rewrapping
  // of the detail text, which is not visually pleasing. To alleviate that
  // issue, always lay out the cell as if there was a Checkmark accessory type.
  //
  // Force the accessory type to Checkmark for the duration of layout.
  MDCCollectionViewCellAccessoryType realAccessoryType = self.accessoryType;
  self.accessoryType = MDCCollectionViewCellAccessoryCheckmark;

  // Implement -layoutSubviews as per instructions in documentation for
  // +[MDCCollectionViewCell cr_preferredHeightForWidth:forItem:].
  [super layoutSubviews];

  // Adjust the text label preferredMaxLayoutWidth when the parent's width
  // changes, for instance on screen rotation.
  CGFloat parentWidth = CGRectGetWidth(self.contentView.bounds);
  self.textLabel.preferredMaxLayoutWidth = parentWidth - 2 * kHorizontalPadding;
  self.detailTextLabel.preferredMaxLayoutWidth =
      parentWidth - 2 * kHorizontalPadding;

  // Re-layout with the new preferred width to allow the label to adjust its
  // height.
  [super layoutSubviews];

  // Restore the real accessory type at the end of the layout.
  self.accessoryType = realAccessoryType;
}

#pragma mark Accessibility

- (NSString*)accessibilityLabel {
  return [NSString stringWithFormat:@"%@, %@", self.textLabel.text,
                                    self.detailTextLabel.text];
}

@end
