// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_ITEM_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_ITEM_H_

#import <UIKit/UIKit.h>

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class ToolsMenuViewCell;

@interface ToolsMenuViewItem : NSObject
@property(nonatomic, copy) NSString* accessibilityIdentifier;
@property(nonatomic, copy) NSString* title;
@property(nonatomic, assign) NSInteger tag;
@property(nonatomic, assign) SEL selector;
@property(nonatomic, assign) BOOL active;
@property(nonatomic, weak) ToolsMenuViewCell* tableViewCell;

+ (NSString*)cellID;
+ (Class)cellClass;

+ (instancetype)menuItemWithTitle:(NSString*)title
          accessibilityIdentifier:(NSString*)accessibilityIdentifier
                         selector:(SEL)selector
                          command:(int)commandID;

// Returns YES if it is valid to call -executeCommandWithDispatcher: on this
// item.
- (BOOL)canExecuteCommand;

// Execute the command associated with this item using |dispatcher|. |selector|
// must be defined on the receiver.
- (void)executeCommandWithDispatcher:
    (id<ApplicationCommands, BrowserCommands>)dispatcher;

@end

@interface ToolsMenuViewCell : UICollectionViewCell
@property(nonatomic, strong) UILabel* title;
@property(nonatomic, readonly) CGFloat horizontalMargin;

- (void)configureForMenuItem:(ToolsMenuViewItem*)item;
- (void)initializeTitleView;
- (void)initializeViews;
@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_ITEM_H_
