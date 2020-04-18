// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/table_view/chrome_table_view_controller.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol PopupMenuItem;
@protocol PopupMenuTableViewControllerCommands;

// TableViewController for the popup menu.
@interface PopupMenuTableViewController : ChromeTableViewController

// The model of this controller.
@property(nonatomic, readonly, strong)
    TableViewModel<TableViewItem<PopupMenuItem>*>* tableViewModel;
// Dispatcher.
@property(nonatomic, weak) id<ApplicationCommands, BrowserCommands> dispatcher;
// Command handler for this table view.
@property(nonatomic, weak) id<PopupMenuTableViewControllerCommands>
    commandHandler;
// Presenting ViewController for the ViewController needing to be presented as
// result of an interaction with the popup.
@property(nonatomic, weak) UIViewController* baseViewController;
// Item to be highlighted. Nil if no item should be highlighted. Must be set
// after the popup menu items.
@property(nonatomic, weak) TableViewItem<PopupMenuItem>* itemToHighlight;

// Initializers.
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithTableViewStyle:(UITableViewStyle)style
                           appBarStyle:
                               (ChromeTableViewControllerStyle)appBarStyle
    NS_UNAVAILABLE;

// Sets the |items| to be displayed by this Table View. Removes all the
// currently presented items.
- (void)setPopupMenuItems:
    (NSArray<NSArray<TableViewItem<PopupMenuItem>*>*>*)items;

@end

#endif  // IOS_CHROME_BROWSER_UI_POPUP_MENU_POPUP_MENU_TABLE_VIEW_CONTROLLER_H_
