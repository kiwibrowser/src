// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class ToolsMenuConfiguration;

// TODO(crbug.com/228521): Remove this once the new command/metric handling is
// implemented. This is a temporary workaround to allow metrics recording to
// distinguish the action. The value used is in the dynamic range (<
// IDC_MinimumLabelValue) to avoid collisions.
#define IDC_TEMP_EDIT_BOOKMARK 3900

// The a11y ID of the tools menu items (used by integration tests).
extern NSString* const kToolsMenuNewTabId;
extern NSString* const kToolsMenuNewIncognitoTabId;
extern NSString* const kToolsMenuCloseAllTabsId;
extern NSString* const kToolsMenuCloseAllIncognitoTabsId;
extern NSString* const kToolsMenuBookmarksId;
extern NSString* const kToolsMenuOtherDevicesId;
extern NSString* const kToolsMenuHistoryId;
extern NSString* const kToolsMenuReportAnIssueId;
extern NSString* const kToolsMenuShareId;
extern NSString* const kToolsMenuDataSavingsId;
extern NSString* const kToolsMenuFindInPageId;
extern NSString* const kToolsMenuRequestDesktopId;
extern NSString* const kToolsMenuSettingsId;
extern NSString* const kToolsMenuHelpId;
extern NSString* const kToolsMenuReadingListId;
extern NSString* const kToolsMenuRequestMobileId;

// Tools Popup Table Delegate Protocol
@protocol ToolsPopupTableDelegate<NSObject>
// Called when a menu item for command |commandID| is selected.
// TODO(stuartmorgan): This is a temporary shim. Remove it once:
// - the automatic command-based metrics system is in place, and
// - we figure out a better way to dismiss the menu (maybe a provided block?)
- (void)commandWasSelected:(int)commandID;
@end

// A table view with two icons in the first row and regular text cells in
// subsequent rows.
// For each icon and item in the menu there is a corresponding delegate method.
@interface ToolsMenuViewController : UIViewController
// Keeps track of the state (Bookmarked or not) of the current visible page.
// This is used to alter the state of the popup menu (i.e. Add/Edit bookmark).
@property(nonatomic, assign) BOOL isCurrentPageBookmarked;
@property(nonatomic, assign) BOOL isTabLoading;
// The tool button to be shown hovering above the popup.
@property(nonatomic, readonly, weak) UIButton* toolsButton;

// Keeps track of the items in tools menu.
@property(nonatomic, copy) NSArray* menuItems;

@property(nonatomic, weak) id<ToolsPopupTableDelegate> delegate;

// Dispatcher for browser commands.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;

// Initializes the Tools popup menu.
- (void)initializeMenuWithConfiguration:(ToolsMenuConfiguration*)configuration;

// Returns the optimal height needed to display the menu items.
// The height returned is usually less than the |suggestedHeight| unless
// the last row of the menu puts the height just over the |suggestedHeight|.
// If the Tools menu items is taller than the |suggestedHeight| by at least
// one menu item, the last visible menu item will be shown partially so user
// can tell that the Tools menu is scrollable.
- (CGFloat)optimalHeight:(CGFloat)suggestedHeight;

// Enable or disable menu item by IDC value.
- (void)setItemEnabled:(BOOL)enabled withTag:(NSInteger)tag;

// Called when the current tab loading state changes.
- (void)setIsTabLoading:(BOOL)isTabLoading;

// TODO(stuartmorgan): Should the set of options that are passed in to the
// constructor just have the ability to specify whether commands should be
// enabled or disabled rather than having these individual setters? crbug/228506
// Informs tools popup menu whether "Find In Page..." command should be
// enabled.
- (void)setCanShowFindBar:(BOOL)enabled;

// Informs tools popup menu whether "Share..." command should be enabled.
- (void)setCanShowShareMenu:(BOOL)enabled;

- (void)animateContentIn;

- (void)hideContent;

// Highlight the New Incognito Tab cell in blue. The highlight fades in, pulses
// once, and fades out.
- (void)triggerNewIncognitoTabCellHighlight;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_VIEW_CONTROLLER_H_
