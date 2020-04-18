// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#import "ios/chrome/browser/ui/commands/application_commands.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kToolsMenuItemHorizontalMargin = 16;
// Increase the margin for RTL so the labels don't overlap the tools icon.
const CGFloat kToolsMenuItemHorizontalMarginRTL = 25;
static NSString* const kMenuItemCellID = @"MenuItemCellID";
}

@implementation ToolsMenuViewItem
@synthesize accessibilityIdentifier = _accessibilityIdentifier;
@synthesize active = _active;
@synthesize title = _title;
@synthesize tag = _tag;
@synthesize selector = _selector;
@synthesize tableViewCell = _tableViewCell;

- (id)init {
  self = [super init];
  if (self) {
    _active = YES;
  }

  return self;
}

+ (NSString*)cellID {
  return kMenuItemCellID;
}

+ (Class)cellClass {
  return [ToolsMenuViewCell class];
}

+ (instancetype)menuItemWithTitle:(NSString*)title
          accessibilityIdentifier:(NSString*)accessibilityIdentifier
                         selector:(SEL)selector
                          command:(int)commandID {
  // Only commandIDs < 0 should have associated selectors.
  DCHECK(selector == nullptr || commandID < 0);
  ToolsMenuViewItem* menuItem = [[self alloc] init];
  [menuItem setAccessibilityLabel:title];
  [menuItem setAccessibilityIdentifier:accessibilityIdentifier];
  [menuItem setTag:commandID];
  [menuItem setSelector:selector];
  [menuItem setTitle:title];

  return menuItem;
}

- (BOOL)canExecuteCommand {
  return self.selector != nullptr;
}

- (void)executeCommandWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher {
  DCHECK(self.selector);
  DCHECK([dispatcher respondsToSelector:self.selector]);
// TODO(crbug.com/738881): Find a better way to call these methods.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [dispatcher performSelector:self.selector];
#pragma clang diagnostic pop
}

@end

@implementation ToolsMenuViewCell
@synthesize title = _title;
@synthesize horizontalMargin = _horizontalMargin;

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  self = [super initWithCoder:aDecoder];
  if (self)
    [self commonInitialization];

  return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self)
    [self commonInitialization];

  return self;
}

- (void)commonInitialization {
  _horizontalMargin = !base::i18n::IsRTL() ? kToolsMenuItemHorizontalMargin
                                           : kToolsMenuItemHorizontalMarginRTL;
  [self setBackgroundColor:[UIColor whiteColor]];
  [self setOpaque:YES];
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [_title setText:nil];
  [self setAccessibilityIdentifier:nil];
  [self setAccessibilityLabel:nil];
}

- (void)initializeTitleView {
  _title = [[UILabel alloc] initWithFrame:self.bounds];
  [_title setTranslatesAutoresizingMaskIntoConstraints:NO];
  [_title setFont:[[MDCTypography fontLoader] regularFontOfSize:16]];
  [_title setBackgroundColor:[self backgroundColor]];
  [_title setTextAlignment:(base::i18n::IsRTL() ? NSTextAlignmentRight
                                                : NSTextAlignmentLeft)];
  [_title setOpaque:YES];
}

- (void)initializeViews {
  if (_title)
    return;

  [self initializeTitleView];

  UIView* contentView = [self contentView];
  [contentView setBackgroundColor:[self backgroundColor]];
  [contentView setOpaque:YES];
  [contentView addSubview:_title];

  NSDictionary* view = @{ @"title" : _title };
  NSString* vertical = @"V:|-(0)-[title]-(0)-|";
  NSString* horizontal = @"H:|-(margin)-[title]-(25)-|";
  NSDictionary* metrics = @{ @"margin" : @(self.horizontalMargin) };

  [contentView
      addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:vertical
                                                             options:0
                                                             metrics:nil
                                                               views:view]];
  [contentView
      addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:horizontal
                                                             options:0
                                                             metrics:metrics
                                                               views:view]];
}

- (void)configureForMenuItem:(ToolsMenuViewItem*)item {
  [self initializeViews];

  UIAccessibilityTraits traits = [self accessibilityTraits] |
                                 UIAccessibilityTraitNotEnabled |
                                 UIAccessibilityTraitButton;
  UIColor* textColor = [UIColor lightGrayColor];
  if ([item active]) {
    textColor = [UIColor blackColor];
    traits &= ~UIAccessibilityTraitNotEnabled;
  }

  [_title setTextColor:textColor];
  [_title setText:[item title]];
  [self setAccessibilityIdentifier:[item accessibilityIdentifier]];
  [self setAccessibilityLabel:[item accessibilityLabel]];
  [self setAccessibilityTraits:traits];
  [self setIsAccessibilityElement:YES];
  [self setTag:[item tag]];
  item.tableViewCell = self;
}

@end
