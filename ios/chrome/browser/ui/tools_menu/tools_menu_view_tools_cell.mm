// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_tools_cell.h"

#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/toolbar/toolbar_resource_macros.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ToolsMenuViewToolsCell

@synthesize reloadButton = _reloadButton;
@synthesize shareButton = _shareButton;
@synthesize starButton = _starButton;
@synthesize starredButton = _starredButton;
@synthesize stopButton = _stopButton;
@synthesize toolsButton = _toolsButton;

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
  [self setBackgroundColor:[UIColor whiteColor]];
  [self setOpaque:YES];

  int star[2][3] = TOOLBAR_IDR_TWO_STATE(STAR);
  _starButton = [self newButtonForImageIds:star
                                 commandID:TOOLS_BOOKMARK_ITEM
                      accessibilityLabelID:IDS_BOOKMARK_ADD_EDITOR_TITLE
                            automationName:@"Add Bookmark"];

  int star_pressed[2][3] = TOOLBAR_IDR_ONE_STATE(STAR_PRESSED);
  _starredButton = [self newButtonForImageIds:star_pressed
                                    commandID:TOOLS_BOOKMARK_EDIT
                         accessibilityLabelID:IDS_IOS_TOOLS_MENU_EDIT_BOOKMARK
                               automationName:@"Edit Bookmark"];

  int reload[2][3] = TOOLBAR_IDR_TWO_STATE(RELOAD);
  _reloadButton = [self newButtonForImageIds:reload
                                   commandID:TOOLS_RELOAD_ITEM
                        accessibilityLabelID:IDS_IOS_ACCNAME_RELOAD
                              automationName:@"Reload"
                               reverseForRTL:YES];

  int stop[2][3] = TOOLBAR_IDR_TWO_STATE(STOP);
  _stopButton = [self newButtonForImageIds:stop
                                 commandID:TOOLS_STOP_ITEM
                      accessibilityLabelID:IDS_IOS_ACCNAME_STOP
                            automationName:@"Stop"];

  int share[2][3] = TOOLBAR_IDR_THREE_STATE(SHARE);
  _shareButton = [self newButtonForImageIds:share
                                  commandID:TOOLS_SHARE_ITEM
                       accessibilityLabelID:IDS_IOS_TOOLS_MENU_SHARE
                             automationName:@"Share"];

  int tools[2][3] = TOOLBAR_IDR_ONE_STATE(TOOLS_PRESSED);
  _toolsButton =
      [self newButtonForImageIds:tools
                       commandID:TOOLS_MENU_ITEM
            accessibilityLabelID:IDS_IOS_TOOLBAR_CLOSE_MENU
                  automationName:@"kToolbarToolsMenuButtonIdentifier"];

  UIView* contentView = [self contentView];
  [contentView setBackgroundColor:[self backgroundColor]];
  [contentView setOpaque:YES];

  [contentView addSubview:_starredButton];
  [contentView addSubview:_starButton];
  [contentView addSubview:_stopButton];
  [contentView addSubview:_reloadButton];
  [contentView addSubview:_shareButton];

  [self addConstraints];
}

- (UIButton*)newButtonForImageIds:(int[2][3])imageIds
                        commandID:(int)commandID
             accessibilityLabelID:(int)labelID
                   automationName:(NSString*)name {
  return [self newButtonForImageIds:imageIds
                          commandID:commandID
               accessibilityLabelID:labelID
                     automationName:name
                      reverseForRTL:NO];
}

- (UIButton*)newButtonForImageIds:(int[2][3])imageIds
                        commandID:(int)commandID
             accessibilityLabelID:(int)labelID
                   automationName:(NSString*)name
                    reverseForRTL:(BOOL)reverseForRTL {
  UIButton* button = [[UIButton alloc] initWithFrame:CGRectZero];
  [button setTranslatesAutoresizingMaskIntoConstraints:NO];

  if (imageIds[0][0]) {
    [button setImage:NativeReversableImage(imageIds[0][0], reverseForRTL)
            forState:UIControlStateNormal];
  }
  [[button imageView] setContentMode:UIViewContentModeCenter];
  [button setBackgroundColor:[self backgroundColor]];
  [button setTag:commandID];
  [button setOpaque:YES];

  SetA11yLabelAndUiAutomationName(button, labelID, name);

  if (imageIds[0][1]) {
    UIImage* pressedImage =
        NativeReversableImage(imageIds[0][1], reverseForRTL);
    if (pressedImage) {
      [button setImage:pressedImage forState:UIControlStateHighlighted];
    }
  }

  if (imageIds[0][2]) {
    UIImage* disabledImage =
        NativeReversableImage(imageIds[0][2], reverseForRTL);
    if (disabledImage) {
      [button setImage:disabledImage forState:UIControlStateDisabled];
    }
  }

  return button;
}

- (void)addConstraints {
  for (UIButton* button in [self allButtons]) {
    NSDictionary* view = @{ @"button" : button };
    NSArray* constraints = @[ @"V:|-(0)-[button]-(0)-|", @"H:[button(==48)]" ];
    ApplyVisualConstraints(constraints, view);
  }

  NSDictionary* views = @{
    @"share" : _shareButton,
    @"star" : _starButton,
    @"reload" : _reloadButton,
    @"starred" : _starredButton,
    @"stop" : _stopButton
  };
  // Leading offset is 16, minus the button image inset of 12.
  NSDictionary* metrics = @{ @"offset" : @4, @"space" : @24 };
  // clang-format off
  NSArray* constraints = @[
    @"H:|-(offset)-[share]-(space)-[star]-(space)-[reload]",
    @"H:[share]-(space)-[starred]",
    @"H:[star]-(space)-[stop]"
  ];
  // clang-format on
  ApplyVisualConstraintsWithMetrics(constraints, views, metrics);
}

// These should be added in display order, so they are animated in display
// order.
- (NSArray*)allButtons {
  NSMutableArray* allButtons = [NSMutableArray array];
  if (_shareButton)
    [allButtons addObject:_shareButton];

  if (_starButton)
    [allButtons addObject:_starButton];

  if (_starredButton)
    [allButtons addObject:_starredButton];

  if (_reloadButton)
    [allButtons addObject:_reloadButton];

  if (_stopButton)
    [allButtons addObject:_stopButton];

  return allButtons;
}

@end
