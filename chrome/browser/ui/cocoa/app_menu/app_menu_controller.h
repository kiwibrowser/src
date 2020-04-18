// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#include "base/time/time.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#import "ui/base/cocoa/menu_controller.h"

class BookmarkMenuBridge;
class Browser;
@class BrowserActionsContainerView;
@class BrowserActionsController;
@class MenuTrackedRootView;
class RecentTabsMenuModelDelegate;
@class ToolbarController;
@class AppMenuButtonViewController;
class AppMenuModel;

namespace app_menu_controller {
// The vertical offset of the app menu bubbles from the app menu button.
extern const CGFloat kAppMenuBubblePointOffsetY;
}

namespace AppMenuControllerInternal {
class AcceleratorDelegate;
class ToolbarActionsBarObserverHelper;
class ZoomLevelObserver;
}  // namespace AppMenuControllerInternal

// The App menu has a creative layout, with buttons in menu items. There is a
// cross-platform model for this special menu, but on the Mac it's easier to
// get spacing and alignment precisely right using a NIB. To do that, we
// subclass the generic MenuControllerCocoa implementation and special-case the
// two items that require specific layout and load them from the NIB.
//
// This object is owned by the ToolbarController and receives its NIB-based
// views using the shim view controller below.
@interface AppMenuController
    : MenuControllerCocoa<NSMenuDelegate, HasWeakBrowserPointer> {
 @private
  // Used to provide accelerators for the menu.
  std::unique_ptr<AppMenuControllerInternal::AcceleratorDelegate>
      acceleratorDelegate_;

  // The model, rebuilt each time the |-menuNeedsUpdate:|.
  std::unique_ptr<AppMenuModel> appMenuModel_;

  // Used to update icons in the recent tabs menu. This must be declared after
  // |appMenuModel_| so that it gets deleted first.
  std::unique_ptr<RecentTabsMenuModelDelegate> recentTabsMenuModelDelegate_;

  // A shim NSViewController that loads the buttons from the NIB because ObjC
  // doesn't have multiple inheritance as this class is a MenuControllerCocoa.
  base::scoped_nsobject<AppMenuButtonViewController> buttonViewController_;

  // The browser for which this controller exists.
  Browser* browser_;  // weak

  // Used to build the bookmark submenu.
  std::unique_ptr<BookmarkMenuBridge> bookmarkMenuBridge_;

  // Observer for page zoom level change notifications.
  std::unique_ptr<AppMenuControllerInternal::ZoomLevelObserver>
      zoom_level_observer_;

  // Observer for the main window's ToolbarActionsBar changing size.
  std::unique_ptr<AppMenuControllerInternal::ToolbarActionsBarObserverHelper>
      toolbar_actions_bar_observer_;

  // The controller for the toolbar actions overflow that is stored in the
  // app menu.
  // This will only be present if the extension action redesign switch is on.
  base::scoped_nsobject<BrowserActionsController> browserActionsController_;

  // The menu item containing the browser actions overflow container.
  NSMenuItem* browserActionsMenuItem_;

  // The time at which the menu was opened.
  base::TimeTicks menuOpenTime_;
}

// Designated initializer.
- (id)initWithBrowser:(Browser*)browser;

// Used to dispatch commands from the App menu. The custom items within the
// menu cannot be hooked up directly to First Responder because the window in
// which the controls reside is not the BrowserWindowController, but a
// NSCarbonMenuWindow; this screws up the typical |-commandDispatch:| system.
- (IBAction)dispatchAppMenuCommand:(id)sender;

// Returns the weak reference to the AppMenuModel.
- (AppMenuModel*)appMenuModel;

// Creates a RecentTabsMenuModelDelegate instance which will take care of
// updating the recent tabs submenu.
- (void)updateRecentTabsSubmenu;

// Updates the browser actions section of the menu.
- (void)updateBrowserActionsSubmenu;

// Retuns the weak reference to the BrowserActionsController.
- (BrowserActionsController*)browserActionsController;

@end

////////////////////////////////////////////////////////////////////////////////

// Shim view controller that merely unpacks objects from a NIB.
@interface AppMenuButtonViewController : NSViewController {
 @private
  AppMenuController* controller_;

  MenuTrackedRootView* editItem_;
  NSButton* editCut_;
  NSButton* editCopy_;
  NSButton* editPaste_;

  MenuTrackedRootView* zoomItem_;
  NSButton* zoomPlus_;
  NSButton* zoomDisplay_;
  NSButton* zoomMinus_;
  NSButton* zoomFullScreen_;

  MenuTrackedRootView* toolbarActionsOverflowItem_;
  BrowserActionsContainerView* overflowActionsContainerView_;
}

@property(retain, nonatomic) IBOutlet MenuTrackedRootView* editItem;
@property(retain, nonatomic) IBOutlet NSButton* editCut;
@property(retain, nonatomic) IBOutlet NSButton* editCopy;
@property(retain, nonatomic) IBOutlet NSButton* editPaste;
@property(retain, nonatomic) IBOutlet MenuTrackedRootView* zoomItem;
@property(retain, nonatomic) IBOutlet NSButton* zoomPlus;
@property(retain, nonatomic) IBOutlet NSButton* zoomDisplay;
@property(retain, nonatomic) IBOutlet NSButton* zoomMinus;
@property(retain, nonatomic) IBOutlet NSButton* zoomFullScreen;
@property(retain, nonatomic)
    IBOutlet MenuTrackedRootView* toolbarActionsOverflowItem;
@property(retain, nonatomic)
    IBOutlet BrowserActionsContainerView* overflowActionsContainerView;

- (id)initWithController:(AppMenuController*)controller;
- (IBAction)dispatchAppMenuCommand:(id)sender;

@end

#endif  // CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_CONTROLLER_H_
