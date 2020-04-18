// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_text_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_cell.h"
#import "ios/chrome/browser/ui/content_suggestions/identifier/content_suggestion_identifier.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContentSuggestionsTextItem

@synthesize text = _text;
@synthesize detailText = _detailText;
@synthesize suggestionIdentifier = _suggestionIdentifier;
@synthesize metricsRecorded = _metricsRecorded;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [CollectionViewTextCell class];
  }
  return self;
}

- (void)configureCell:(CollectionViewTextCell*)cell {
  [super configureCell:cell];

  [self configureTextLabel:cell.textLabel];
  [self configureDetailTextLabel:cell.detailTextLabel];

  cell.isAccessibilityElement = YES;
  if (self.detailText.length == 0) {
    cell.accessibilityLabel = self.text;
  } else {
    cell.accessibilityLabel =
        [NSString stringWithFormat:@"%@, %@", self.text, self.detailText];
  }
}

- (CGFloat)cellHeightForWidth:(CGFloat)width {
  UILabel* textLabel = [[UILabel alloc] init];
  UILabel* detailTextLabel = [[UILabel alloc] init];
  [self configureTextLabel:textLabel];
  [self configureDetailTextLabel:detailTextLabel];

  return [self.cellClass heightForTitleLabel:textLabel
                             detailTextLabel:detailTextLabel
                                       width:width];
}

#pragma mark - Private

// Configures the |textLabel|.
- (void)configureTextLabel:(UILabel*)textLabel {
  textLabel.text = self.text;
  textLabel.textColor = [[MDCPalette greyPalette] tint900];
  textLabel.font = [MDCTypography body2Font];
  textLabel.numberOfLines = 0;
}

// Configures the |detailTextLabel|.
- (void)configureDetailTextLabel:(UILabel*)detailTextLabel {
  detailTextLabel.text = self.detailText;
  detailTextLabel.textColor = [[MDCPalette greyPalette] tint500];
  detailTextLabel.font = [MDCTypography body1Font];
  detailTextLabel.numberOfLines = 0;
}

@end
