// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_MEDIATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/popup_menu/public/popup_menu_ui_updating.h"

namespace bookmarks {
class BookmarkModel;
}
namespace feature_engagement {
class Tracker;
}
@protocol BrowserCommands;
@class PopupMenuTableViewController;
class ReadingListModel;
class WebStateList;

// Mediator for the popup menu. This object is in charge of creating and
// updating the items of the popup menu.
@interface PopupMenuMediator : NSObject

// Initializes the mediator with a |type| of popup menu, whether it
// |isIncognito|, a |readingListModel| used to display the badge for the reading
// list entry, and whether the mediator should |triggerNewIncognitoTabTip|.
- (instancetype)initWithType:(PopupMenuType)type
                  isIncognito:(BOOL)isIncognito
             readingListModel:(ReadingListModel*)readingListModel
    triggerNewIncognitoTabTip:(BOOL)triggerNewIncognitoTabTip
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// The WebStateList that this mediator listens for any changes on the current
// WebState.
@property(nonatomic, assign) WebStateList* webStateList;
// The TableView to be configured with this mediator.
@property(nonatomic, strong) PopupMenuTableViewController* popupMenu;
// Dispatcher.
@property(nonatomic, weak) id<BrowserCommands> dispatcher;
// Records events for the use of in-product help. The mediator does not take
// ownership of tracker. Tracker must not be destroyed during lifetime of the
// object.
@property(nonatomic, assign) feature_engagement::Tracker* engagementTracker;
// The bookmarks model to know if the page is bookmarked.
@property(nonatomic, assign) bookmarks::BookmarkModel* bookmarkModel;

// Disconnect the mediator.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_MEDIATOR_H_
