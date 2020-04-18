// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item.h"

#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_cell.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_item_accessibility_delegate.h"
#import "ios/chrome/browser/ui/util/pasteboard_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#import "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - ReadingListCollectionViewItem

@interface ReadingListCollectionViewItem () {
  GURL _url;
}

// Returns the accessibility custom actions associated with this item.
- (NSArray<UIAccessibilityCustomAction*>*)customActions;

@end

@implementation ReadingListCollectionViewItem
@synthesize attributes = _attributes;
@synthesize title = _title;
@synthesize subtitle = _subtitle;
@synthesize url = _url;
@synthesize faviconPageURL = _faviconPageURL;
@synthesize distillationState = _distillationState;
@synthesize distillationDate = _distillationDate;
@synthesize distillationSize = _distillationSize;
@synthesize accessibilityDelegate = _accessibilityDelegate;

- (instancetype)initWithType:(NSInteger)type
                         url:(const GURL&)url
           distillationState:(ReadingListUIDistillationStatus)state {
  self = [super initWithType:type];
  if (!self)
    return nil;
  self.cellClass = [ReadingListCell class];
  _url = url;
  _distillationState = state;
  return self;
}

#pragma mark - CollectionViewTextItem

- (void)configureCell:(ReadingListCell*)cell {
  [super configureCell:cell];
  [cell.faviconView configureWithAttributes:self.attributes];
  cell.titleLabel.text = self.title;
  cell.subtitleLabel.text = self.subtitle;
  cell.distillationState = _distillationState;
  cell.distillationSize = _distillationSize;
  cell.distillationDate = _distillationDate;
  cell.isAccessibilityElement = YES;
  cell.accessibilityLabel = [self accessibilityLabel];
  cell.accessibilityCustomActions = [self customActions];
}

#pragma mark - Private

- (NSString*)accessibilityLabel {
  NSString* accessibilityState = nil;
  if (self.distillationState == ReadingListUIDistillationStatusSuccess) {
    accessibilityState = l10n_util::GetNSString(
        IDS_IOS_READING_LIST_ACCESSIBILITY_STATE_DOWNLOADED);
  } else {
    accessibilityState = l10n_util::GetNSString(
        IDS_IOS_READING_LIST_ACCESSIBILITY_STATE_NOT_DOWNLOADED);
  }

  return l10n_util::GetNSStringF(IDS_IOS_READING_LIST_ENTRY_ACCESSIBILITY_LABEL,
                                 base::SysNSStringToUTF16(self.title),
                                 base::SysNSStringToUTF16(accessibilityState),
                                 base::SysNSStringToUTF16(self.subtitle));
}

#pragma mark - AccessibilityCustomAction

- (NSArray<UIAccessibilityCustomAction*>*)customActions {
  UIAccessibilityCustomAction* deleteAction = [
      [UIAccessibilityCustomAction alloc]
      initWithName:l10n_util::GetNSString(IDS_IOS_READING_LIST_DELETE_BUTTON)
            target:self
          selector:@selector(deleteEntry)];
  UIAccessibilityCustomAction* toogleReadStatus = nil;
  if ([self.accessibilityDelegate isEntryRead:self]) {
    toogleReadStatus = [[UIAccessibilityCustomAction alloc]
        initWithName:l10n_util::GetNSString(
                         IDS_IOS_READING_LIST_MARK_UNREAD_BUTTON)
              target:self
            selector:@selector(markUnread)];
  } else {
    toogleReadStatus = [[UIAccessibilityCustomAction alloc]
        initWithName:l10n_util::GetNSString(
                         IDS_IOS_READING_LIST_MARK_READ_BUTTON)
              target:self
            selector:@selector(markRead)];
  }

  UIAccessibilityCustomAction* openInNewTabAction =
      [[UIAccessibilityCustomAction alloc]
          initWithName:l10n_util::GetNSString(
                           IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWTAB)
                target:self
              selector:@selector(openInNewTab)];
  UIAccessibilityCustomAction* openInNewIncognitoTabAction =
      [[UIAccessibilityCustomAction alloc]
          initWithName:l10n_util::GetNSString(
                           IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWINCOGNITOTAB)
                target:self
              selector:@selector(openInNewIncognitoTab)];
  UIAccessibilityCustomAction* copyURLAction =
      [[UIAccessibilityCustomAction alloc]
          initWithName:l10n_util::GetNSString(IDS_IOS_CONTENT_CONTEXT_COPY)
                target:self
              selector:@selector(copyURL)];

  NSMutableArray* customActions = [NSMutableArray
      arrayWithObjects:deleteAction, toogleReadStatus, openInNewTabAction,
                       openInNewIncognitoTabAction, copyURLAction, nil];

  if (self.distillationState == ReadingListUIDistillationStatusSuccess) {
    // Add the possibility to open offline version only if the entry is
    // distilled.
    UIAccessibilityCustomAction* openOfflineAction =
        [[UIAccessibilityCustomAction alloc]
            initWithName:l10n_util::GetNSString(
                             IDS_IOS_READING_LIST_CONTENT_CONTEXT_OFFLINE)
                  target:self
                selector:@selector(openOffline)];

    [customActions addObject:openOfflineAction];
  }

  return customActions;
}

- (BOOL)deleteEntry {
  [self.accessibilityDelegate deleteEntry:self];
  return YES;
}

- (BOOL)markRead {
  [self.accessibilityDelegate markEntryRead:self];
  return YES;
}

- (BOOL)markUnread {
  [self.accessibilityDelegate markEntryUnread:self];
  return YES;
}

- (BOOL)openInNewTab {
  [self.accessibilityDelegate openEntryInNewTab:self];
  return YES;
}

- (BOOL)openInNewIncognitoTab {
  [self.accessibilityDelegate openEntryInNewIncognitoTab:self];
  return YES;
}

- (BOOL)copyURL {
  StoreURLInPasteboard(self.url);
  return YES;
}

- (BOOL)openOffline {
  [self.accessibilityDelegate openEntryOffline:self];
  return YES;
}

#pragma mark - NSObject

- (NSString*)description {
  return [NSString stringWithFormat:@"Reading List item \"%@\" for url %@",
                                    self.title, self.subtitle];
}

- (BOOL)isEqual:(id)other {
  if (other == self)
    return YES;
  if (!other || ![other isKindOfClass:[self class]])
    return NO;
  ReadingListCollectionViewItem* otherItem =
      static_cast<ReadingListCollectionViewItem*>(other);
  return [self.title isEqualToString:otherItem.title] &&
         [self.subtitle isEqualToString:otherItem.subtitle] &&
         self.distillationState == otherItem.distillationState &&
         self.distillationSize == otherItem.distillationSize &&
         self.distillationDate == otherItem.distillationDate;
}

@end
