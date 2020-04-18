// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/legacy_history_entries_status_item.h"

#include "base/mac/foundation_util.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/history/history_entries_status_item_delegate.h"
#import "ios/chrome/browser/ui/util/label_link_controller.h"
#import "ios/chrome/common/string_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Delegate for HistoryEntriesStatusCell.
@protocol HistoryEntriesStatusCellDelegate<NSObject>
// Notifies the delegate that |URL| should be opened.
- (void)historyEntriesStatusCell:(LegacyHistoryEntriesStatusCell*)cell
               didRequestOpenURL:(const GURL&)URL;
@end

@interface LegacyHistoryEntriesStatusCell ()
// Redeclare as readwrite.
@property(nonatomic, strong, readwrite)
    LabelLinkController* labelLinkController;
// Delegate for the HistoryEntriesStatusCell. Is notified when a link is
// tapped.
@property(nonatomic, weak) id<HistoryEntriesStatusCellDelegate> delegate;
// Sets the browsing data url link on the cell label.
- (void)setLinkForBrowsingDataURL:(const GURL&)browsingDataURL;
@end

@interface LegacyHistoryEntriesStatusItem ()<HistoryEntriesStatusCellDelegate>
@end

@implementation LegacyHistoryEntriesStatusItem
@synthesize delegate = _delegate;
@synthesize hidden = _hidden;

- (Class)cellClass {
  return [LegacyHistoryEntriesStatusCell class];
}

- (void)configureCell:(LegacyHistoryEntriesStatusCell*)cell {
  [super configureCell:cell];
  [cell setDelegate:self];
  if (self.hidden) {
    cell.textLabel.text = nil;
  } else {
    cell.textLabel.text =
        l10n_util::GetNSString(IDS_IOS_HISTORY_OTHER_FORMS_OF_HISTORY);
    ;
    [cell setLinkForBrowsingDataURL:GURL(kHistoryMyActivityURL)];
  }
}

- (void)historyEntriesStatusCell:(LegacyHistoryEntriesStatusCell*)cell
               didRequestOpenURL:(const GURL&)URL {
  [self.delegate historyEntriesStatusItem:self didRequestOpenURL:URL];
}

- (BOOL)isEqualToHistoryEntriesStatusItem:
    (LegacyHistoryEntriesStatusItem*)object {
  return self.hidden == object.hidden;
}

- (BOOL)isEqual:(id)object {
  if (self == object) {
    return YES;
  }
  if (![object isKindOfClass:[LegacyHistoryEntriesStatusItem class]]) {
    return NO;
  }
  return [self
      isEqualToHistoryEntriesStatusItem:base::mac::ObjCCastStrict<
                                            LegacyHistoryEntriesStatusItem>(
                                            object)];
}

@end

@implementation LegacyHistoryEntriesStatusCell
@synthesize delegate = _delegate;
@synthesize labelLinkController = _labelLinkController;

- (void)setLinkForBrowsingDataURL:(const GURL&)browsingDataURL {
  __weak LegacyHistoryEntriesStatusCell* weakSelf = self;
  self.labelLinkController = [[LabelLinkController alloc]
      initWithLabel:self.textLabel
             action:^(const GURL& URL) {
               [[weakSelf delegate] historyEntriesStatusCell:weakSelf
                                           didRequestOpenURL:URL];
             }];
  [self.labelLinkController setLinkColor:[[MDCPalette cr_bluePalette] tint500]];

  // Remove link delimiter from text and get ranges for links. Must be parsed
  // before being added to the controller because modifying the label text
  // clears all added links.
  NSRange otherBrowsingDataRange;
  if (browsingDataURL.is_valid()) {
    self.textLabel.text =
        ParseStringWithLink(self.textLabel.text, &otherBrowsingDataRange);
    DCHECK(otherBrowsingDataRange.location != NSNotFound &&
           otherBrowsingDataRange.length);

    [self.labelLinkController addLinkWithRange:otherBrowsingDataRange
                                           url:browsingDataURL];
  }
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.labelLinkController = nil;
  self.delegate = nil;
}

@end
