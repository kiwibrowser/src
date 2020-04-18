// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"
#import "chrome/browser/ui/cocoa/view_resizer.h"
#include "components/prefs/pref_member.h"
#import "ui/base/cocoa/tracking_area.h"

@class AutocompleteTextField;
@class AutocompleteTextFieldEditor;
@class BackForwardMenuController;
class Browser;
@class BrowserActionsContainerView;
@class BrowserActionsController;
class CommandUpdater;
class LocationBarViewMac;
@class MenuButton;
class Profile;
@class ReloadButton;
@class ToolbarButton;
@class ToolbarView;
@class AppMenuController;

namespace content {
class WebContents;
}

namespace ToolbarControllerInternal {
class CommandObserverBridge;
class NotificationBridge;
}

// A controller for the toolbar in the browser window. Manages
// updating the state for location bar and back/fwd/reload/go buttons.
// Manages the bookmark bar and its position in the window relative to
// the web content view.

@interface ToolbarController
    : NSViewController<URLDropTargetController, HasWeakBrowserPointer> {
 @protected
  // The ordering is important for unit tests. If new items are added or the
  // ordering is changed, make sure to update |-toolbarViews| and the
  // corresponding enum in the unit tests.
  IBOutlet MenuButton* backButton_;
  IBOutlet MenuButton* forwardButton_;
  IBOutlet ReloadButton* reloadButton_;
  IBOutlet ToolbarButton* homeButton_;
  IBOutlet MenuButton* appMenuButton_;
  IBOutlet AutocompleteTextField* locationBar_;
  IBOutlet BrowserActionsContainerView* browserActionsContainerView_;

 @private
  CommandUpdater* commands_;  // weak, one per window
  Profile* profile_;  // weak, one per window
  Browser* browser_;  // weak, one per window
  std::unique_ptr<ToolbarControllerInternal::CommandObserverBridge>
      commandObserver_;
  std::unique_ptr<LocationBarViewMac> locationBarView_;
  base::scoped_nsobject<AutocompleteTextFieldEditor>
      autocompleteTextFieldEditor_;
  base::scoped_nsobject<BackForwardMenuController> backMenuController_;
  base::scoped_nsobject<BackForwardMenuController> forwardMenuController_;
  base::scoped_nsobject<BrowserActionsController> browserActionsController_;

  // Lazily-instantiated menu controller.
  base::scoped_nsobject<AppMenuController> appMenuController_;

  // Used for monitoring the optional toolbar button prefs.
  std::unique_ptr<ToolbarControllerInternal::NotificationBridge>
      notificationBridge_;
  BooleanPrefMember showHomeButton_;
  BOOL hasToolbar_;  // If NO, we may have only the location bar.
  BOOL hasLocationBar_;  // If |hasToolbar_| is YES, this must also be YES.
  BOOL locationBarAtMinSize_; // If the location bar is at the minimum size.

  // Tracking area for mouse enter/exit/moved in the toolbar.
  ui::ScopedCrTrackingArea trackingArea_;

  // We retain/release the hover button since interaction with the
  // button may make it go away (e.g. delete menu option over a
  // bookmark button).  Thus this variable is not weak.  The
  // hoveredButton_ is required to have an NSCell that responds to
  // setMouseInside:animate:.
  NSButton* hoveredButton_;
}

// Return the inset needed to center a toolbar button's icon within the 28x28
// design area defined by Material Design.
+ (CGFloat)materialDesignButtonInset;

// Initialize the toolbar and register for command updates. The profile is
// needed for initializing the location bar. The browser is needed for
// the toolbar model and back/forward menus.
- (id)initWithCommands:(CommandUpdater*)commands
               profile:(Profile*)profile
               browser:(Browser*)browser;

// Strongly typed controlled view.
- (ToolbarView*)toolbarView;

// Get the C++ bridge object representing the location bar for this tab.
- (LocationBarViewMac*)locationBarBridge;

// Called by the Window delegate so we can provide a custom field editor if
// needed.
// Note that this may be called for objects unrelated to the toolbar.
// returns nil if we don't want to override the custom field editor for |obj|.
- (id)customFieldEditorForObject:(id)obj;

// Called by the |locationBar_| when it has been added to its window.
- (void)locationBarWasAddedToWindow;

// Return YES if the location bar is the first responder.
- (BOOL)locationBarHasFocus;

// Make the location bar the first responder, if possible.
- (void)focusLocationBar:(BOOL)selectAll;

// Called by CommandObserverBridge when there is a state change for the given
// command.
- (void)enabledStateChangedForCommand:(int)command enabled:(bool)enabled;

// Forces the toolbar (and transitively the location bar) to update its current
// state.  If |tab| is non-NULL, we're switching (back?) to this tab and should
// restore any previous location bar state (such as user editing) as well.
- (void)updateToolbarWithContents:(content::WebContents*)tab;

// Resets the state for |tab|.
- (void)resetTabState:(content::WebContents*)tab;

// Sets whether or not the current page in the frontmost tab is bookmarked.
- (void)setStarredState:(BOOL)isStarred;

// Sets whether or not the current page is translated.
- (void)setTranslateIconLit:(BOOL)on;

// Happens when the zoom for the active tab changes, the active tab switches, or
// a new tab or browser window is created. |canShowBubble| indicates if it is
// appropriate to show a zoom bubble for the change.
- (void)zoomChangedForActiveTab:(BOOL)canShowBubble;

// Called to update the loading state. Handles updating the go/stop
// button state.  |force| is set if the update is due to changing
// tabs, as opposed to the page-load finishing.  See comment in
// reload_button_cocoa.h.
- (void)setIsLoading:(BOOL)isLoading force:(BOOL)force;

// Allow turning off the toolbar (but we may keep the location bar without a
// surrounding toolbar). If |toolbar| is YES, the value of |hasLocationBar| is
// ignored. This changes the behavior of other methods, like |-view|.
- (void)setHasToolbar:(BOOL)toolbar hasLocationBar:(BOOL)locBar;

// Point on the star icon for the bookmark bubble to be - in the
// associated window's coordinate system.
- (NSPoint)bookmarkBubblePoint;

// Point on the save credit card icon for the save credit card bubble.
- (NSPoint)saveCreditCardBubblePoint;

// Point in the window's coordinate system for bubbles attached to the app menu.
- (NSPoint)appMenuBubblePoint;

// Returns the desired toolbar height for the given compression factor.
- (CGFloat)desiredHeightForCompression:(CGFloat)compressByHeight;

// Set the opacity of the divider (the line at the bottom) *if* we have a
// |ToolbarView| (0 means don't show it); no-op otherwise.
- (void)setDividerOpacity:(CGFloat)opacity;

// Create and add the Browser Action buttons to the toolbar view.
- (void)createBrowserActionButtons;

// Updates the visibility of the toolbar, with an optional animation.
- (void)updateVisibility:(BOOL)visible withAnimation:(BOOL)animate;

// Return the BrowserActionsController for this toolbar.
- (BrowserActionsController*)browserActionsController;

// Returns the app menu button.
- (NSButton*)appMenuButton;

// Returns the app menu controller.
- (AppMenuController*)appMenuController;

// Returns true of the location bar is focused.
- (BOOL)isLocationBarFocused;

@end

// A set of private methods used by tests, in the absence of "friends" in ObjC.
@interface ToolbarController(PrivateTestMethods)
// Returns an array of views in the order of the outlets above.
- (NSArray*)toolbarViews;
- (void)showOptionalHomeButton;
- (void)installAppMenu;
// Return a hover button for the current event.
- (NSButton*)hoverButtonForEvent:(NSEvent*)theEvent;
// Adjusts browser actions container view in response to toolbar frame changes.
// Outside of tests, called in response to frame changed/new window
// notifications.
- (void)toolbarFrameChanged;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_CONTROLLER_H_
