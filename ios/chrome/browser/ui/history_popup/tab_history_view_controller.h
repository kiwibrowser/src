// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "ios/web/public/navigation_item_list.h"

@protocol TabHistoryPopupCommands;

// View controller for displaying a list of NavigationItems in a table.
@interface TabHistoryViewController : UICollectionViewController

// Designated initializer that takes a NavigationItemList.
- (instancetype)initWithItems:(const web::NavigationItemList&)items
                   dispatcher:(id<TabHistoryPopupCommands>)dispatcher
    NS_DESIGNATED_INITIALIZER;

// TabHistoryViewControllers must be initialized with |-initWithItems:|.
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithCollectionViewLayout:(UICollectionViewLayout*)layout
    NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString*)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil NS_UNAVAILABLE;

// Returns the optimal height needed to display the session entries.
// The height returned is usually less than the |suggestedHeight| unless
// the last row of the table puts the height just over the |suggestedHeight|.
// If the session entries table is taller than the |suggestedHeight| by at least
// one row, the last visible session entry will be shown partially so the user
// can tell that the table is scrollable.
- (CGFloat)optimalHeight:(CGFloat)suggestedHeight;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_VIEW_CONTROLLER_H_
