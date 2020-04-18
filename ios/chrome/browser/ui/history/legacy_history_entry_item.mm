// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/legacy_history_entry_item.h"

#include "base/i18n/time_formatting.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/history/core/browser/url_row.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_service_factory.h"
#import "ios/chrome/browser/ui/history/favicon_view.h"
#import "ios/chrome/browser/ui/history/favicon_view_provider.h"
#import "ios/chrome/browser/ui/history/history_entry_item_delegate.h"
#include "ios/chrome/browser/ui/history/history_util.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Size at which the favicon will be displayed.
const CGFloat kFaviconSize = 24.0;
// Minimum size at which to fetch favicons.
const CGFloat kMinFaviconSize = 16.0;
// Horizontal spacing between edge of the cell and the cell content.
const CGFloat kMargin = 16.0;
// Horizontal spacing between the leading edge of the cell and the text.
const CGFloat kHeaderMargin = 56.0;
}  // namespace

#pragma mark - LegacyHistoryEntryItem

@interface LegacyHistoryEntryItem ()<FaviconViewProviderDelegate> {
  // Delegate for LegacyHistoryEntryItem.
  __weak id<HistoryEntryItemDelegate> _delegate;
}

// FaviconViewProvider to fetch the favicon and format the favicon view.
@property(nonatomic, strong) FaviconViewProvider* faviconViewProvider;

// Custom accessibility actions for the history entry view.
- (NSArray*)accessibilityActions;
// Custom accessibility action to delete the history entry.
- (BOOL)deleteHistoryEntry;
// Custom accessibility action to open the history entry's URL in a new tab.
- (BOOL)openInNewTab;
// Custom accessibility action to open the history entry's URL in a new
// incognito tab.
- (BOOL)openInNewIncognitoTab;
// Custom accessibility action to copy the history entry's URL to the clipboard.
- (BOOL)copyURL;
@end

@implementation LegacyHistoryEntryItem

@synthesize faviconViewProvider = _faviconViewProvider;
@synthesize text = _text;
@synthesize detailText = _detailText;
@synthesize timeText = _timeText;
@synthesize URL = _URL;
@synthesize timestamp = _timestamp;

- (instancetype)initWithType:(NSInteger)type
                historyEntry:
                    (const history::BrowsingHistoryService::HistoryEntry&)entry
                browserState:(ios::ChromeBrowserState*)browserState
                    delegate:(id<HistoryEntryItemDelegate>)delegate {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [LegacyHistoryEntryCell class];
    favicon::LargeIconService* largeIconService =
        IOSChromeLargeIconServiceFactory::GetForBrowserState(browserState);
    _faviconViewProvider =
        [[FaviconViewProvider alloc] initWithURL:entry.url
                                     faviconSize:kFaviconSize
                                  minFaviconSize:kMinFaviconSize
                                largeIconService:largeIconService
                                        delegate:self];
    _text = [history::FormattedTitle(entry.title, entry.url) copy];
    _detailText = [base::SysUTF8ToNSString(entry.url.spec()) copy];
    _timeText =
        [base::SysUTF16ToNSString(base::TimeFormatTimeOfDay(entry.time)) copy];
    _URL = GURL(entry.url);
    _timestamp = entry.time;
    _delegate = delegate;
  }
  return self;
}

- (instancetype)initWithType:(NSInteger)type {
  NOTREACHED();
  return nil;
}

- (BOOL)isEqualToHistoryEntryItem:(id<HistoryEntryItemInterface>)item {
  return item && item.URL == _URL && item.timestamp == _timestamp;
}

- (BOOL)isEqual:(id)object {
  if (self == object)
    return YES;

  if (![object isMemberOfClass:[LegacyHistoryEntryItem class]])
    return NO;

  return [self isEqualToHistoryEntryItem:object];
}

- (NSUInteger)hash {
  return [base::SysUTF8ToNSString(self.URL.spec()) hash] ^
         self.timestamp.ToInternalValue();
}

- (NSArray*)accessibilityActions {
  UIAccessibilityCustomAction* deleteAction =
      [[UIAccessibilityCustomAction alloc]
          initWithName:l10n_util::GetNSString(
                           IDS_HISTORY_ENTRY_ACCESSIBILITY_DELETE)
                target:self
              selector:@selector(deleteHistoryEntry)];
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
  return @[
    deleteAction, openInNewTabAction, openInNewIncognitoTabAction, copyURLAction
  ];
}

- (BOOL)deleteHistoryEntry {
  [_delegate historyEntryItemDidRequestDelete:self];
  return YES;
}

- (BOOL)openInNewTab {
  [_delegate historyEntryItemDidRequestOpenInNewTab:self];
  return YES;
}

- (BOOL)openInNewIncognitoTab {
  [_delegate historyEntryItemDidRequestOpenInNewIncognitoTab:self];
  return YES;
}

- (BOOL)copyURL {
  [_delegate historyEntryItemDidRequestCopy:self];
  return YES;
}

- (void)configureCell:(LegacyHistoryEntryCell*)cell {
  [super configureCell:cell];

  // Set favicon view and constraints.
  FaviconView* faviconView = self.faviconViewProvider.faviconView;
  [cell.faviconViewContainer addSubview:faviconView];
  [faviconView setTranslatesAutoresizingMaskIntoConstraints:NO];
  AddSameConstraints(faviconView, cell.faviconViewContainer);

  cell.textLabel.text = self.text;
  cell.detailTextLabel.text = self.detailText;
  cell.timeLabel.text = self.timeText;
  cell.isAccessibilityElement = YES;
  cell.accessibilityCustomActions = self.accessibilityActions;
  cell.accessibilityLabel =
      l10n_util::GetNSStringF(IDS_HISTORY_ENTRY_ACCESSIBILITY_LABEL,
                              base::SysNSStringToUTF16(self.text),
                              base::SysNSStringToUTF16(self.detailText),
                              base::SysNSStringToUTF16(self.timeText));
}

- (void)faviconViewProviderFaviconDidLoad:(FaviconViewProvider*)provider {
  [_delegate historyEntryItemShouldUpdateView:self];
}

@end

#pragma mark - LegacyHistoryEntryCell

@interface LegacyHistoryEntryCell ()

// Redeclare as readwrite.
@property(nonatomic, readwrite, strong) UILabel* textLabel;
@property(nonatomic, readwrite, strong) UILabel* detailTextLabel;
@property(nonatomic, readwrite, strong) UILabel* timeLabel;
@end

@implementation LegacyHistoryEntryCell

@synthesize faviconViewContainer = _faviconViewContainer;
@synthesize textLabel = _textLabel;
@synthesize detailTextLabel = _detailTextLabel;
@synthesize timeLabel = _timeLabel;

- (id)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _faviconViewContainer = [[UIView alloc] initWithFrame:CGRectZero];

    _textLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    [_textLabel setFont:[[MDCTypography fontLoader] mediumFontOfSize:16]];
    [_textLabel setTextColor:[[MDCPalette greyPalette] tint900]];

    _detailTextLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    [_detailTextLabel
        setFont:[[MDCTypography fontLoader] regularFontOfSize:14]];
    [_detailTextLabel setTextColor:[[MDCPalette greyPalette] tint600]];

    _timeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    [_timeLabel setFont:[[MDCTypography fontLoader] mediumFontOfSize:14]];
    [_timeLabel setTextColor:[[MDCPalette greyPalette] tint600]];
    _timeLabel.textAlignment =
        UseRTLLayout() ? NSTextAlignmentLeft : NSTextAlignmentRight;

    UIView* contentView = self.contentView;
    [contentView addSubview:_faviconViewContainer];
    [contentView addSubview:_textLabel];
    [contentView addSubview:_detailTextLabel];
    [contentView addSubview:_timeLabel];

    [_faviconViewContainer setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_textLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_detailTextLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_timeLabel setTranslatesAutoresizingMaskIntoConstraints:NO];

    [_faviconViewContainer.widthAnchor constraintEqualToConstant:kFaviconSize];

    NSDictionary* views = @{
      @"title" : _textLabel,
      @"URL" : _detailTextLabel,
      @"time" : _timeLabel,
      @"favicon" : _faviconViewContainer,
    };
    NSDictionary* metrics = @{
      @"margin" : @(kMargin),
      @"spacing" : @(kHeaderMargin - (kMargin + kFaviconSize)),
    };
    NSArray* constraints = @[
      @"H:|-margin-[favicon]-spacing-[title]-[time]-margin-|",
      @"H:|-margin-[favicon]-spacing-[URL]-margin-|",
      @"V:|-margin-[title][URL]-margin-|",
    ];
    ApplyVisualConstraintsWithMetrics(constraints, views, metrics);
    AddSameCenterYConstraint(_textLabel, _timeLabel);
    AddSameCenterYConstraint(_faviconViewContainer, _textLabel);

    [_timeLabel
        setContentCompressionResistancePriority:UILayoutPriorityRequired
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];
  }
  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  _textLabel.text = nil;
  _detailTextLabel.text = nil;
  _timeLabel.text = nil;
  for (UIView* subview in _faviconViewContainer.subviews) {
    [subview removeFromSuperview];
  }
}

@end
