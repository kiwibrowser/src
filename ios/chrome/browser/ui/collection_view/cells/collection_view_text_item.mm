// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_cell.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation CollectionViewTextItem

@synthesize accessoryType = _accessoryType;
@synthesize text = _text;
@synthesize detailText = _detailText;
@synthesize textFont = _textFont;
@synthesize textColor = _textColor;
@synthesize numberOfTextLines = _numberOfTextLines;
@synthesize detailTextFont = _detailTextFont;
@synthesize detailTextColor = _detailTextColor;
@synthesize numberOfDetailTextLines = _numberOfDetailTextLines;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [CollectionViewTextCell class];
    _numberOfTextLines = 1;
    _numberOfDetailTextLines = 1;
  }
  return self;
}

- (UIFont*)textFont {
  if (!_textFont) {
    _textFont = [[MDCTypography fontLoader] mediumFontOfSize:14];
  }
  return _textFont;
}

- (UIColor*)textColor {
  if (!_textColor) {
    _textColor = [[MDCPalette greyPalette] tint900];
  }
  return _textColor;
}

- (UIFont*)detailTextFont {
  if (!_detailTextFont) {
    _detailTextFont = [[MDCTypography fontLoader] regularFontOfSize:14];
  }
  return _detailTextFont;
}

- (UIColor*)detailTextColor {
  if (!_detailTextColor) {
    _detailTextColor = [[MDCPalette greyPalette] tint500];
  }
  return _detailTextColor;
}

#pragma mark CollectionViewItem

- (void)configureCell:(CollectionViewTextCell*)cell {
  [super configureCell:cell];
  [cell cr_setAccessoryType:self.accessoryType];
  cell.textLabel.text = self.text;
  cell.detailTextLabel.text = self.detailText;
  cell.isAccessibilityElement = YES;
  if ([self.accessibilityLabel length] != 0) {
    cell.accessibilityLabel = self.accessibilityLabel;
  } else {
    if (self.detailText.length == 0) {
      cell.accessibilityLabel = self.text;
    } else {
      cell.accessibilityLabel =
          [NSString stringWithFormat:@"%@, %@", self.text, self.detailText];
    }
  }

  // Styling.
  cell.textLabel.font = self.textFont;
  cell.textLabel.textColor = self.textColor;
  cell.textLabel.numberOfLines = self.numberOfTextLines;
  cell.detailTextLabel.font = self.detailTextFont;
  cell.detailTextLabel.textColor = self.detailTextColor;
  cell.detailTextLabel.numberOfLines = self.numberOfDetailTextLines;
}

@end
