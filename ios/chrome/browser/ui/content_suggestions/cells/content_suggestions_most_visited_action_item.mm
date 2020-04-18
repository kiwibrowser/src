// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_action_item.h"

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_action_cell.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ContentSuggestionsMostVisitedActionItem

@synthesize action = _action;
@synthesize count = _count;
@synthesize metricsRecorded = _metricsRecorded;
@synthesize suggestionIdentifier = _suggestionIdentifier;
@synthesize title = _title;

- (instancetype)initWithAction:(ContentSuggestionsMostVisitedAction)action {
  self = [super initWithType:0];
  if (self) {
    _action = action;
    self.cellClass = [ContentSuggestionsMostVisitedActionCell class];
    self.title = [self titleForAction:_action];
  }
  return self;
}

#pragma mark - AccessibilityCustomAction

- (void)configureCell:(ContentSuggestionsMostVisitedActionCell*)cell {
  [super configureCell:cell];
  cell.accessibilityCustomActions = nil;
  cell.titleLabel.text = self.title;
  cell.accessibilityLabel = self.title;
  cell.iconView.image = [self imageForAction:_action];
  if (self.count != 0) {
    cell.countLabel.text = [@(self.count) stringValue];
    cell.countContainer.hidden = NO;
  } else {
    cell.countContainer.hidden = YES;
  }
}

#pragma mark - ContentSuggestionsItem

- (CGFloat)cellHeightForWidth:(CGFloat)width {
  return [ContentSuggestionsMostVisitedActionCell defaultSize].height;
}

#pragma mark - Private

- (NSString*)titleForAction:(ContentSuggestionsMostVisitedAction)action {
  switch (action) {
    case ContentSuggestionsMostVisitedActionBookmark:
      return l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_BOOKMARKS);
    case ContentSuggestionsMostVisitedActionReadingList:
      return l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_READING_LIST);
    case ContentSuggestionsMostVisitedActionRecentTabs:
      return l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_RECENT_TABS);
    case ContentSuggestionsMostVisitedActionHistory:
      return l10n_util::GetNSString(IDS_IOS_CONTENT_SUGGESTIONS_HISTORY);
  }
}

- (UIImage*)imageForAction:(ContentSuggestionsMostVisitedAction)action {
  switch (action) {
    case ContentSuggestionsMostVisitedActionBookmark:
      return [UIImage imageNamed:@"ntp_bookmarks_icon"];
    case ContentSuggestionsMostVisitedActionReadingList:
      return [UIImage imageNamed:@"ntp_readinglist_icon"];
    case ContentSuggestionsMostVisitedActionRecentTabs:
      return [UIImage imageNamed:@"ntp_recent_icon"];
    case ContentSuggestionsMostVisitedActionHistory:
      return [UIImage imageNamed:@"ntp_history_icon"];
  }
}

@end
