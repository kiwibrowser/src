// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class ReadingListMenuNotifier;

namespace web {
enum class UserAgentType : short;
}  // namespace web

namespace feature_engagement {
class Tracker;
}  // namespace feature_engagement

// Configuation defining options that can be set to change the tools menu's
// appearance. All boolean properties are set to NO, and |userAgentType| is set
// to NONE by default.
@interface ToolsMenuConfiguration : NSObject
// Indicates that the menu is being shown while in the tab switcher.
@property(nonatomic, getter=isInTabSwitcher) BOOL inTabSwitcher;
// Indicates that the menu is being shown while there are no open tabs.
@property(nonatomic, getter=hasNoOpenedTabs) BOOL noOpenedTabs;
// Indicates that the menu is being shown while in incognito mode.
@property(nonatomic, getter=isInIncognito) BOOL inIncognito;
// Indicates that the menu should show a "NEW" badge on the reading list item.
@property(nonatomic, assign) BOOL showReadingListNewBadge;
// Indicates that the New Incognito Tab cell should be highlighted in blue.
@property(nonatomic, assign) BOOL highlightNewIncognitoTabCell;

// Indicates that the menu is being shown while user agent is |userAgentType|.
// If NONE, shows "Request Desktop Site" in disabled state.
// If MOBILE, shows "Request Desktop Site" in enabled state.
// If DESKTOP, shows "Request Mobile Site" in enabled state.
@property(nonatomic, assign) web::UserAgentType userAgentType;

// The view controller from which to present other view controllers.
@property(nonatomic, weak, readonly) UIViewController* baseViewController;
// View that the menu will be displayed in.
@property(nonatomic, weak, readonly) UIView* displayView;
// Button from which popup menu will be opened.
@property(nonatomic, weak) UIButton* toolsMenuButton;
// Menu's origin relative to the |displayView|'s coordinate system, calculated
// from |toolsMenuButton| and |displayView|.
@property(nonatomic, readonly) CGRect sourceRect;
// Image insets that should be applied to the tools button if it is displayed,
// calculated from |toolsMenuButton|.
@property(nonatomic, readonly) UIEdgeInsets toolsButtonInsets;
// Notifier for changes to the reading list requiring the menu to be updated.
// Menus needing to be updated should set themselves as this object's delegate.
@property(nonatomic, weak) ReadingListMenuNotifier* readingListMenuNotifier;
// Records the time that the tools menu was requested; value is the time
// interval since the NSDate referenceDate.
@property(nonatomic, assign) NSTimeInterval requestStartTime;
// Records events for the use of in-product help. Does not take ownership of
// tracker. Tracker must not be destroyed during lifetime of
// ToolsMenuConfiguration. Defaults to |nullptr|.
@property(nonatomic, assign) feature_engagement::Tracker* engagementTracker;

// Initialize a ToolsMenuContext instance with default values. |displayView| is
// the weakly-held parent view within which the popup tools menu using this
// context will be displayed. |baseViewController| is a weakly-held view
// controller from which to present other view controllers.
- (instancetype)initWithDisplayView:(UIView*)displayView
                 baseViewController:(UIViewController*)baseViewController
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONFIGURATION_H_
