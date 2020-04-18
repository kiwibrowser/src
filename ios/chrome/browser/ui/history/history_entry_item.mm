// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/history_entry_item.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/history/core/browser/browsing_history_service.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_url_item.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - HistoryEntryItem

@implementation HistoryEntryItem
@synthesize text = _text;
@synthesize detailText = _detailText;
@synthesize timeText = _timeText;
@synthesize URL = _URL;
@synthesize timestamp = _timestamp;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [TableViewURLCell class];
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];

  TableViewURLCell* cell =
      base::mac::ObjCCastStrict<TableViewURLCell>(tableCell);
  cell.titleLabel.text = self.text;
  cell.URLLabel.text = self.detailText;
  cell.metadataLabel.text = self.timeText;
  cell.metadataLabel.hidden = ([self.timeText length] == 0);
  cell.faviconView.backgroundColor = styler.tableViewBackgroundColor;
  cell.titleLabel.backgroundColor = styler.tableViewBackgroundColor;
  cell.URLLabel.backgroundColor = styler.tableViewBackgroundColor;
  cell.metadataLabel.backgroundColor = styler.tableViewBackgroundColor;
}

#pragma mark NSObject

- (BOOL)isEqualToHistoryEntryItem:(HistoryEntryItem*)item {
  return item && item.URL == _URL && item.timestamp == _timestamp;
}

- (BOOL)isEqual:(id)object {
  if (self == object)
    return YES;

  if (![object isMemberOfClass:[HistoryEntryItem class]])
    return NO;

  return [self isEqualToHistoryEntryItem:object];
}

- (NSUInteger)hash {
  return [base::SysUTF8ToNSString(self.URL.spec()) hash] ^
         self.timestamp.since_origin().InMicroseconds();
}

@end
