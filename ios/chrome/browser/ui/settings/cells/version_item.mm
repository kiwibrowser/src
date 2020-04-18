// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/version_item.h"

#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation VersionItem

@synthesize text = _text;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.accessibilityIdentifier = @"Version cell";
    self.cellClass = [VersionCell class];
  }
  return self;
}

#pragma mark CollectionViewItem

- (void)configureCell:(VersionCell*)cell {
  [super configureCell:cell];
  cell.textLabel.text = self.text;
}

@end

@implementation VersionCell

@synthesize textLabel = _textLabel;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.isAccessibilityElement = YES;
    self.accessibilityTraits |= UIAccessibilityTraitButton;
    self.accessibilityHint = l10n_util::GetNSString(IDS_IOS_COPY_VERSION_HINT);

    _textLabel = [[UILabel alloc] init];
    _textLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _textLabel.backgroundColor = [UIColor clearColor];
    _textLabel.textAlignment = NSTextAlignmentCenter;
    _textLabel.shadowOffset = CGSizeMake(1, 0);
    _textLabel.shadowColor = [UIColor whiteColor];
    [self addSubview:_textLabel];

    _textLabel.font = [[MDCTypography fontLoader] mediumFontOfSize:14];
    _textLabel.textColor = [[MDCPalette greyPalette] tint900];

    // Set up the constraints.
    [NSLayoutConstraint activateConstraints:@[
      [_textLabel.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
      [_textLabel.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    ]];

    self.shouldHideSeparator = YES;
  }
  return self;
}

- (NSString*)accessibilityLabel {
  return self.textLabel.text;
}

@end
