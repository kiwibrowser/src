// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/copied_to_chrome_item.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
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
}  // namespace

@implementation CopiedToChromeItem

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [CopiedToChromeCell class];
  }
  return self;
}

@end

@implementation CopiedToChromeCell

@synthesize textLabel = _textLabel;
@synthesize button = _button;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    UIView* contentView = self.contentView;

    _textLabel = [[UILabel alloc] init];
    _textLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_textLabel];

    _button = [[MDCFlatButton alloc] init];
    _button.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_button];

    _textLabel.font = [[MDCTypography fontLoader] mediumFontOfSize:14];
    _textLabel.textColor = [[MDCPalette greyPalette] tint900];
    _textLabel.text =
        l10n_util::GetNSString(IDS_IOS_AUTOFILL_DESCRIBE_LOCAL_COPY);

    [_button setTitleColor:[[MDCPalette cr_bluePalette] tint600]
                  forState:UIControlStateNormal];
    [_button
        setTitle:l10n_util::GetNSString(IDS_AUTOFILL_CLEAR_LOCAL_COPY_BUTTON)
        forState:UIControlStateNormal];

    // Set up the constraints.
    [NSLayoutConstraint activateConstraints:@[
      [_textLabel.leadingAnchor
          constraintEqualToAnchor:contentView.leadingAnchor
                         constant:kHorizontalPadding],
      [_textLabel.topAnchor constraintEqualToAnchor:contentView.topAnchor
                                           constant:kVerticalPadding],
      [_textLabel.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor
                                              constant:-kVerticalPadding],
      [_button.trailingAnchor
          constraintEqualToAnchor:contentView.trailingAnchor],
      [_button.centerYAnchor constraintEqualToAnchor:contentView.centerYAnchor],
    ]];
  }
  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [self.button removeTarget:nil
                     action:nil
           forControlEvents:UIControlEventAllEvents];
}

@end
