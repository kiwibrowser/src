// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/content_suggestions/sc_content_suggestions_most_visited_item.h"

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_cell.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SCContentSuggestionsMostVisitedItem

@synthesize attributes;
@synthesize suggestionIdentifier;
@synthesize title;
@synthesize metricsRecorded;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [ContentSuggestionsMostVisitedCell class];
  }
  return self;
}

- (void)configureCell:(ContentSuggestionsMostVisitedCell*)cell {
  [super configureCell:cell];
  cell.titleLabel.text = self.title;
  cell.accessibilityLabel = self.title;
  [cell.faviconView configureWithAttributes:self.attributes];
}

- (CGFloat)cellHeightForWidth:(CGFloat)width {
  return [ContentSuggestionsMostVisitedCell defaultSize].height;
}

@end
